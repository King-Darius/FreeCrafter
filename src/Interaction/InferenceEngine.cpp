#include "InferenceEngine.h"

#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/GeometryObject.h"
#include "../GeometryKernel/HalfEdgeMesh.h"
#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <unordered_set>

namespace Interaction {
namespace {

constexpr float kEpsilon = 1e-5f;
constexpr float kPointSnapRadius = 0.12f;
constexpr float kMidpointSnapRadius = 0.15f;
constexpr float kEdgeSnapRadius = 0.18f;
constexpr float kFaceSnapRadius = 0.24f;
constexpr float kParallelThreshold = 0.9848f; // cos(10 deg)
constexpr float kPerpendicularThreshold = 0.0872f; // sin(5 deg)

float component(const Vector3& v, int axis)
{
    switch (axis) {
    case 0: return v.x;
    case 1: return v.y;
    default: return v.z;
    }
}

Vector3 normalizeOrZero(const Vector3& v)
{
    float len = v.length();
    if (len <= kEpsilon) {
        return Vector3();
    }
    return v / len;
}

float distancePointToRaySquared(const Vector3& point, const PickRay& ray, float& outRayT)
{
    Vector3 diff = point - ray.origin;
    outRayT = diff.dot(ray.direction);
    if (outRayT < 0.0f) {
        outRayT = 0.0f;
    }
    Vector3 projected = ray.origin + ray.direction * outRayT;
    Vector3 delta = point - projected;
    return delta.lengthSquared();
}

float distancePointToRaySquared(const Vector3& point, const PickRay& ray)
{
    float t = 0.0f;
    return distancePointToRaySquared(point, ray, t);
}

float distanceToBoundsSquared(const Vector3& point, const Vector3& minBounds, const Vector3& maxBounds)
{
    float dx = 0.0f;
    if (point.x < minBounds.x) dx = minBounds.x - point.x;
    else if (point.x > maxBounds.x) dx = point.x - maxBounds.x;
    float dy = 0.0f;
    if (point.y < minBounds.y) dy = minBounds.y - point.y;
    else if (point.y > maxBounds.y) dy = point.y - maxBounds.y;
    float dz = 0.0f;
    if (point.z < minBounds.z) dz = minBounds.z - point.z;
    else if (point.z > maxBounds.z) dz = point.z - maxBounds.z;
    return dx*dx + dy*dy + dz*dz;
}

struct LineIntersection {
    bool valid = false;
    Vector3 pointOnRay;
    Vector3 pointOnSegment;
    float rayT = 0.0f;
    float segmentT = 0.0f;
    float distanceSquared = std::numeric_limits<float>::infinity();
};

LineIntersection closestBetweenRayAndSegment(const PickRay& ray, const Vector3& a, const Vector3& b)
{
    LineIntersection result;
    Vector3 v = b - a;
    Vector3 w0 = ray.origin - a;
    float aDot = ray.direction.dot(ray.direction);
    float bDot = ray.direction.dot(v);
    float cDot = v.dot(v);
    float dDot = ray.direction.dot(w0);
    float eDot = v.dot(w0);
    float denom = aDot * cDot - bDot * bDot;
    float s = 0.0f;
    float t = 0.0f;
    if (std::fabs(denom) < kEpsilon) {
        // almost parallel, project onto segment
        t = std::clamp(-eDot / (cDot > kEpsilon ? cDot : 1.0f), 0.0f, 1.0f);
        s = 0.0f;
    } else {
        s = (bDot * eDot - cDot * dDot) / denom;
        t = (aDot * eDot - bDot * dDot) / denom;
        if (t < 0.0f) {
            t = 0.0f;
            s = -dDot / (aDot > kEpsilon ? aDot : 1.0f);
        } else if (t > 1.0f) {
            t = 1.0f;
            s = (bDot - dDot) / (aDot > kEpsilon ? aDot : 1.0f);
        }
        if (s < 0.0f) s = 0.0f;
    }
    result.pointOnRay = ray.origin + ray.direction * s;
    result.pointOnSegment = a + v * t;
    Vector3 delta = result.pointOnSegment - result.pointOnRay;
    result.distanceSquared = delta.lengthSquared();
    result.rayT = s;
    result.segmentT = t;
    result.valid = true;
    return result;
}

float biasForType(InferenceSnapType type)
{
    switch (type) {
    case InferenceSnapType::Endpoint: return 0.1f;
    case InferenceSnapType::Intersection: return 0.05f;
    case InferenceSnapType::Midpoint: return 0.2f;
    case InferenceSnapType::FaceCenter: return 0.25f;
    case InferenceSnapType::OnEdge: return 0.3f;
    case InferenceSnapType::Parallel: return 0.32f;
    case InferenceSnapType::Perpendicular: return 0.34f;
    case InferenceSnapType::OnFace: return 0.4f;
    case InferenceSnapType::Axis: return 0.0f;
    default: return 1.0f;
    }
}

long long makeUndirectedKey(int a, int b)
{
    if (a > b) std::swap(a, b);
    return (static_cast<long long>(a) << 32) | static_cast<unsigned int>(b);
}

} // namespace

struct InferenceEngine::FaceFeature {
    Vector3 center;
    Vector3 normal;
    float radiusSquared = 0.0f;
};

struct InferenceEngine::EdgeFeature {
    Vector3 a;
    Vector3 b;
    Vector3 midpoint;
    Vector3 direction;
};

class InferenceEngine::SimpleKdTree {
public:
    void build(const std::vector<Vector3>* pts)
    {
        points = pts;
        indices.clear();
        nodes.clear();
        if (!points || points->empty()) {
            return;
        }
        indices.resize(points->size());
        std::iota(indices.begin(), indices.end(), 0);
        nodes.reserve(points->size() * 2);
        buildRecursive(0, static_cast<int>(indices.size()));
    }

    int nearest(const Vector3& target, float maxDistSq) const
    {
        if (!points || points->empty() || nodes.empty()) {
            return -1;
        }
        int bestIndex = -1;
        float bestDistSq = maxDistSq;
        nearestRecursive(0, target, bestDistSq, bestIndex);
        return bestIndex;
    }

private:
    struct Node {
        int axis = -1;
        int left = -1;
        int right = -1;
        int start = 0;
        int end = 0;
        int mid = -1;
        float split = 0.0f;
        Vector3 minBounds;
        Vector3 maxBounds;
    };

    const std::vector<Vector3>* points = nullptr;
    std::vector<int> indices;
    std::vector<Node> nodes;

    int buildRecursive(int start, int end)
    {
        Node node;
        node.start = start;
        node.end = end;
        node.left = -1;
        node.right = -1;
        node.mid = -1;

        Vector3 minB = (*points)[indices[start]];
        Vector3 maxB = minB;
        for (int i = start + 1; i < end; ++i) {
            const Vector3& p = (*points)[indices[i]];
            if (p.x < minB.x) minB.x = p.x;
            if (p.y < minB.y) minB.y = p.y;
            if (p.z < minB.z) minB.z = p.z;
            if (p.x > maxB.x) maxB.x = p.x;
            if (p.y > maxB.y) maxB.y = p.y;
            if (p.z > maxB.z) maxB.z = p.z;
        }
        node.minBounds = minB;
        node.maxBounds = maxB;

        int count = end - start;
        int nodeIndex = static_cast<int>(nodes.size());
        nodes.push_back(node);

        if (count <= 12) {
            nodes[nodeIndex] = node;
            return nodeIndex;
        }

        Vector3 extents(maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z);
        int axis = 0;
        if (extents.y > extents.x && extents.y >= extents.z) axis = 1;
        else if (extents.z > extents.x && extents.z >= extents.y) axis = 2;

        int mid = start + count / 2;
        auto begin = indices.begin() + start;
        auto nth = indices.begin() + mid;
        auto endIt = indices.begin() + end;
        std::nth_element(begin, nth, endIt, [&](int lhs, int rhs) {
            return component((*points)[lhs], axis) < component((*points)[rhs], axis);
        });

        nodes[nodeIndex].axis = axis;
        nodes[nodeIndex].mid = mid;
        nodes[nodeIndex].split = component((*points)[indices[mid]], axis);
        nodes[nodeIndex].minBounds = minB;
        nodes[nodeIndex].maxBounds = maxB;
        nodes[nodeIndex].left = buildRecursive(start, mid);
        nodes[nodeIndex].right = buildRecursive(mid, end);
        return nodeIndex;
    }

    void nearestRecursive(int nodeIndex, const Vector3& target, float& bestDistSq, int& bestIndex) const
    {
        const Node& node = nodes[nodeIndex];
        if (node.axis < 0 || node.mid < 0) {
            for (int i = node.start; i < node.end; ++i) {
                int idx = indices[i];
                const Vector3& p = (*points)[idx];
                float distSq = (p - target).lengthSquared();
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestIndex = idx;
                }
            }
            return;
        }

        float diff = component(target, node.axis) - node.split;
        int first = diff <= 0.0f ? node.left : node.right;
        int second = diff <= 0.0f ? node.right : node.left;

        if (first >= 0) {
            nearestRecursive(first, target, bestDistSq, bestIndex);
        }
        if (second >= 0) {
            const Node& other = nodes[second];
            float bboxDist = distanceToBoundsSquared(target, other.minBounds, other.maxBounds);
            if (bboxDist <= bestDistSq) {
                nearestRecursive(second, target, bestDistSq, bestIndex);
            }
        }
    }
};

InferenceEngine::InferenceEngine()
    : endpointTree(new SimpleKdTree())
    , intersectionTree(new SimpleKdTree())
    , midpointTree(new SimpleKdTree())
    , faceCenterTree(new SimpleKdTree())
    , cachedStamp(0)
    , dirty(true)
{
}

void InferenceEngine::invalidate()
{
    dirty = true;
}

std::size_t InferenceEngine::computeStamp(const GeometryKernel& geometry) const
{
    std::size_t stamp = 1469598103934665603ull;
    const auto& objects = geometry.getObjects();
    for (const auto& obj : objects) {
        const HalfEdgeMesh& mesh = obj->getMesh();
        stamp ^= mesh.getVertices().size() + 0x9e3779b97f4a7c15ull + (stamp << 6) + (stamp >> 2);
        stamp ^= mesh.getHalfEdges().size() + 0x517cc1b727220a95ull + (stamp << 6) + (stamp >> 2);
        stamp ^= mesh.getFaces().size() + 0x27d4eb2f165667c5ull + (stamp << 6) + (stamp >> 2);
    }
    stamp ^= objects.size();
    return stamp;
}

void InferenceEngine::ensureIndex(const GeometryKernel& geometry)
{
    std::size_t stamp = computeStamp(geometry);
    if (!dirty && stamp == cachedStamp) {
        return;
    }
    cachedStamp = stamp;
    rebuild(geometry);
    dirty = false;
}

void InferenceEngine::rebuild(const GeometryKernel& geometry)
{
    endpointPositions.clear();
    intersectionPositions.clear();
    midpointPositions.clear();
    faceFeatures.clear();
    edgeFeatures.clear();
    faceCenterPositions.clear();

    const auto& objects = geometry.getObjects();
    for (const auto& obj : objects) {
        const HalfEdgeMesh& mesh = obj->getMesh();
        const auto& vertices = mesh.getVertices();
        const auto& halfEdges = mesh.getHalfEdges();
        const auto& faces = mesh.getFaces();

        if (vertices.empty()) {
            continue;
        }

        std::unordered_set<long long> edgeSeen;
        std::vector<int> valence(vertices.size(), 0);
        for (const auto& he : halfEdges) {
            if (he.origin >= 0 && he.origin < static_cast<int>(valence.size())) {
                ++valence[static_cast<size_t>(he.origin)];
            }
        }

        for (const auto& v : vertices) {
            endpointPositions.push_back(v.position);
        }

        for (size_t i = 0; i < vertices.size(); ++i) {
            if (valence[i] >= 3) {
                intersectionPositions.push_back(vertices[i].position);
            }
        }

        for (const auto& he : halfEdges) {
            if (he.origin < 0 || he.destination < 0) continue;
            long long key = makeUndirectedKey(he.origin, he.destination);
            if (!edgeSeen.insert(key).second) continue;
            const Vector3& a = vertices[static_cast<size_t>(he.origin)].position;
            const Vector3& b = vertices[static_cast<size_t>(he.destination)].position;
            Vector3 midpoint = (a + b) * 0.5f;
            midpointPositions.push_back(midpoint);
            EdgeFeature feature;
            feature.a = a;
            feature.b = b;
            feature.midpoint = midpoint;
            feature.direction = normalizeOrZero(b - a);
            edgeFeatures.push_back(feature);
        }

        for (const auto& face : faces) {
            if (face.halfEdge < 0 || face.halfEdge >= static_cast<int>(halfEdges.size())) {
                continue;
            }
            Vector3 centroid;
            int count = 0;
            float maxRadiusSq = 0.0f;
            int heIndex = face.halfEdge;
            std::unordered_set<int> visited;
            while (visited.insert(heIndex).second) {
                const auto& he = halfEdges[static_cast<size_t>(heIndex)];
                if (he.origin < 0 || he.origin >= static_cast<int>(vertices.size())) {
                    break;
                }
                const Vector3& pos = vertices[static_cast<size_t>(he.origin)].position;
                centroid += pos;
                ++count;
                heIndex = he.next;
                if (heIndex < 0 || heIndex >= static_cast<int>(halfEdges.size())) {
                    break;
                }
                if (count > static_cast<int>(vertices.size()) * 2) {
                    break; // guard
                }
            }
            if (count <= 0) {
                continue;
            }
            centroid = centroid / static_cast<float>(count);
            heIndex = face.halfEdge;
            visited.clear();
            while (visited.insert(heIndex).second) {
                const auto& he = halfEdges[static_cast<size_t>(heIndex)];
                if (he.origin < 0 || he.origin >= static_cast<int>(vertices.size())) {
                    break;
                }
                const Vector3& pos = vertices[static_cast<size_t>(he.origin)].position;
                float dSq = (pos - centroid).lengthSquared();
                if (dSq > maxRadiusSq) maxRadiusSq = dSq;
                heIndex = he.next;
                if (heIndex < 0 || heIndex >= static_cast<int>(halfEdges.size())) {
                    break;
                }
                if (visited.size() > vertices.size() * 2) {
                    break;
                }
            }
            FaceFeature feature;
            feature.center = centroid;
            feature.normal = face.normal;
            feature.radiusSquared = maxRadiusSq;
            faceCenterPositions.push_back(centroid);
            faceFeatures.push_back(feature);
        }
    }

    endpointTree->build(&endpointPositions);
    intersectionTree->build(&intersectionPositions);
    midpointTree->build(&midpointPositions);
    faceCenterTree->build(&faceCenterPositions);
}

InferenceResult InferenceEngine::query(const GeometryKernel& geometry, const InferenceContext& context)
{
    ensureIndex(geometry);

    InferenceResult none;
    if (context.maxSnapDistance <= 0.0f) {
        return none;
    }

    PickRay ray = context.ray;
    float dirLen = ray.direction.length();
    if (dirLen <= kEpsilon) {
        return none;
    }
    ray.direction = ray.direction / dirLen;

    std::vector<InferenceResult> candidates;
    candidates.reserve(12);

    const std::array<float, 3> samples = { 1.0f, 5.0f, 20.0f };

    auto trySample = [&](const std::vector<Vector3>& positions, const std::unique_ptr<SimpleKdTree>& tree,
                         InferenceSnapType type, float threshold) {
        if (!tree || positions.empty()) return;
        for (float s : samples) {
            Vector3 probe = ray.origin + ray.direction * s;
            int idx = tree->nearest(probe, std::numeric_limits<float>::max());
            if (idx < 0 || idx >= static_cast<int>(positions.size())) continue;
            float t = 0.0f;
            float distSq = distancePointToRaySquared(positions[static_cast<size_t>(idx)], ray, t);
            if (distSq > threshold * threshold) continue;
            InferenceResult res;
            res.type = type;
            res.position = positions[static_cast<size_t>(idx)];
            res.distance = distSq;
            candidates.push_back(res);
        }
    };

    trySample(endpointPositions, endpointTree, InferenceSnapType::Endpoint, kPointSnapRadius);
    trySample(intersectionPositions, intersectionTree, InferenceSnapType::Intersection, kPointSnapRadius);
    trySample(midpointPositions, midpointTree, InferenceSnapType::Midpoint, kMidpointSnapRadius);
    trySample(faceCenterPositions, faceCenterTree, InferenceSnapType::FaceCenter, kFaceSnapRadius);

    // edge-based suggestions
    for (const auto& edge : edgeFeatures) {
        LineIntersection info = closestBetweenRayAndSegment(ray, edge.a, edge.b);
        if (!info.valid) continue;
        if (info.distanceSquared > kEdgeSnapRadius * kEdgeSnapRadius) continue;
        InferenceResult res;
        res.type = InferenceSnapType::OnEdge;
        res.position = info.pointOnSegment;
        res.direction = edge.direction;
        res.reference = edge.a;
        res.distance = info.distanceSquared;
        candidates.push_back(res);

        float alignment = std::fabs(edge.direction.dot(ray.direction));
        if (alignment > kParallelThreshold) {
            InferenceResult parallel = res;
            parallel.type = InferenceSnapType::Parallel;
            parallel.distance = info.distanceSquared * 1.05f;
            candidates.push_back(parallel);
        } else if (alignment < kPerpendicularThreshold) {
            InferenceResult perp = res;
            perp.type = InferenceSnapType::Perpendicular;
            perp.distance = info.distanceSquared * 1.1f;
            candidates.push_back(perp);
        }
    }

    // face suggestions
    for (size_t i = 0; i < faceFeatures.size(); ++i) {
        const FaceFeature& face = faceFeatures[i];
        float denom = face.normal.dot(ray.direction);
        if (std::fabs(denom) < kEpsilon) {
            continue;
        }
        float t = face.normal.dot(face.center - ray.origin) / denom;
        if (t < 0.0f) continue;
        Vector3 hit = ray.origin + ray.direction * t;
        float distSq = (hit - face.center).lengthSquared();
        float radiusSq = face.radiusSquared + std::max(0.01f, face.radiusSquared * 0.15f);
        if (distSq <= radiusSq) {
            InferenceResult res;
            res.type = InferenceSnapType::OnFace;
            res.position = hit;
            res.direction = face.normal;
            res.reference = face.center;
            res.distance = distSq;
            candidates.push_back(res);
        }

        float centerDist = distancePointToRaySquared(face.center, ray);
        if (centerDist <= kFaceSnapRadius * kFaceSnapRadius) {
            InferenceResult res;
            res.type = InferenceSnapType::FaceCenter;
            res.position = face.center;
            res.direction = face.normal;
            res.reference = face.center;
            res.distance = centerDist;
            candidates.push_back(res);
        }
    }

    if (candidates.empty()) {
        return none;
    }

    float bestScore = std::numeric_limits<float>::infinity();
    InferenceResult best = none;
    for (const auto& candidate : candidates) {
        if (candidate.distance > context.maxSnapDistance * context.maxSnapDistance) {
            continue;
        }
        float score = candidate.distance + biasForType(candidate.type);
        if (score < bestScore) {
            bestScore = score;
            best = candidate;
        }
    }

    return best;
}

const char* toString(InferenceSnapType type)
{
    switch (type) {
    case InferenceSnapType::Endpoint: return "Endpoint";
    case InferenceSnapType::Midpoint: return "Midpoint";
    case InferenceSnapType::Intersection: return "Intersection";
    case InferenceSnapType::FaceCenter: return "Face center";
    case InferenceSnapType::OnEdge: return "On edge";
    case InferenceSnapType::OnFace: return "On face";
    case InferenceSnapType::Axis: return "Axis";
    case InferenceSnapType::Parallel: return "Parallel";
    case InferenceSnapType::Perpendicular: return "Perpendicular";
    default: return "";
    }
}

} // namespace Interaction
