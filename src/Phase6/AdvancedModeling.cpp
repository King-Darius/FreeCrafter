#include "AdvancedModeling.h"

#include "../GeometryKernel/HalfEdgeMesh.h"
#include "../GeometryKernel/MeshUtils.h"
#include "../GeometryKernel/TransformUtils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace Phase6 {
namespace {

constexpr float kEpsilon = 1e-5f;

long long makeUndirectedEdge(int a, int b)
{
    if (a > b)
        std::swap(a, b);
    return (static_cast<long long>(a) << 32) | static_cast<unsigned int>(b);
}

std::vector<Vector3> closeLoop(const std::vector<Vector3>& input)
{
    if (input.empty())
        return {};
    std::vector<Vector3> loop = input;
    if ((loop.front() - loop.back()).length() > kEpsilon) {
        loop.push_back(loop.front());
    }
    return loop;
}

std::vector<Vector3> resampleLoop(const std::vector<Vector3>& loop, std::size_t samples)
{
    if (loop.size() < 3)
        return {};
    samples = std::max<std::size_t>(3, samples);
    std::vector<Vector3> closed = closeLoop(loop);
    if (closed.size() < 2)
        return {};

    std::vector<float> cumulative(closed.size(), 0.0f);
    float totalLength = 0.0f;
    for (std::size_t i = 1; i < closed.size(); ++i) {
        totalLength += (closed[i] - closed[i - 1]).length();
        cumulative[i] = totalLength;
    }
    if (totalLength <= kEpsilon)
        return loop;

    std::vector<Vector3> result;
    result.reserve(samples);
    float step = totalLength / static_cast<float>(samples);
    for (std::size_t s = 0; s < samples; ++s) {
        float target = step * static_cast<float>(s);
        auto it = std::lower_bound(cumulative.begin(), cumulative.end(), target);
        std::size_t index = std::min<std::size_t>(std::distance(cumulative.begin(), it), closed.size() - 1);
        if (index == 0) {
            result.push_back(closed.front());
            continue;
        }
        float segStart = cumulative[index - 1];
        float segEnd = cumulative[index];
        float segLength = std::max(kEpsilon, segEnd - segStart);
        float t = (target - segStart) / segLength;
        const Vector3& a = closed[index - 1];
        const Vector3& b = closed[index];
        result.push_back(a * (1.0f - t) + b * t);
    }
    return result;
}

std::vector<Vector3> resamplePath(const std::vector<Vector3>& path, std::size_t samples)
{
    if (path.size() < 2)
        return path;
    samples = std::max<std::size_t>(2, samples);
    std::vector<float> cumulative(path.size(), 0.0f);
    float totalLength = 0.0f;
    for (std::size_t i = 1; i < path.size(); ++i) {
        totalLength += (path[i] - path[i - 1]).length();
        cumulative[i] = totalLength;
    }
    if (totalLength <= kEpsilon)
        return path;

    std::vector<Vector3> result;
    result.reserve(samples);
    float step = totalLength / static_cast<float>(samples - 1);
    for (std::size_t s = 0; s < samples; ++s) {
        float target = std::min(totalLength, step * static_cast<float>(s));
        auto it = std::lower_bound(cumulative.begin(), cumulative.end(), target);
        std::size_t index = std::min<std::size_t>(std::distance(cumulative.begin(), it), path.size() - 1);
        if (index == 0) {
            result.push_back(path.front());
            continue;
        }
        float segStart = cumulative[index - 1];
        float segEnd = cumulative[index];
        float segLength = std::max(kEpsilon, segEnd - segStart);
        float t = (target - segStart) / segLength;
        const Vector3& a = path[index - 1];
        const Vector3& b = path[index];
        result.push_back(a * (1.0f - t) + b * t);
    }
    return result;
}

Vector3 computeCentroid(const std::vector<Vector3>& points)
{
    if (points.empty())
        return Vector3(0.0f, 0.0f, 0.0f);
    Vector3 sum(0.0f, 0.0f, 0.0f);
    for (const auto& p : points)
        sum += p;
    return sum / static_cast<float>(points.size());
}

Vector3 perpendicularVector(const Vector3& axis)
{
    Vector3 reference = std::fabs(axis.x) < std::fabs(axis.y) ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
    Vector3 perp = axis.cross(reference);
    if (perp.lengthSquared() <= kEpsilon)
        perp = axis.cross(Vector3(0.0f, 0.0f, 1.0f));
    if (perp.lengthSquared() <= kEpsilon)
        return Vector3(1.0f, 0.0f, 0.0f);
    return perp.normalized();
}

Vector3 closestPointOnSegment(const Vector3& point, const Vector3& a, const Vector3& b)
{
    Vector3 ab = b - a;
    float denom = ab.lengthSquared();
    if (denom <= kEpsilon)
        return a;
    float t = (point - a).dot(ab) / denom;
    t = std::clamp(t, 0.0f, 1.0f);
    return a + ab * t;
}

Vector3 closestPointOnTriangle(const Vector3& point, const Vector3& a, const Vector3& b, const Vector3& c)
{
    // Barycentric technique from Real-Time Collision Detection (Ericson)
    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 ap = point - a;
    float d1 = ab.dot(ap);
    float d2 = ac.dot(ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        return a;

    Vector3 bp = point - b;
    float d3 = ab.dot(bp);
    float d4 = ac.dot(bp);
    if (d3 >= 0.0f && d4 <= d3)
        return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return a + ab * v;
    }

    Vector3 cp = point - c;
    float d5 = ab.dot(cp);
    float d6 = ac.dot(cp);
    if (d6 >= 0.0f && d5 <= d6)
        return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return a + ac * w;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        Vector3 bc = c - b;
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + bc * w;
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

float distanceToPolyline(const Vector3& point, const std::vector<Vector3>& polyline)
{
    if (polyline.size() < 2)
        return (point - polyline.front()).length();
    float best = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i + 1 < polyline.size(); ++i) {
        Vector3 candidate = closestPointOnSegment(point, polyline[i], polyline[i + 1]);
        float dist = (candidate - point).length();
        if (dist < best)
            best = dist;
    }
    return best;
}

Vector3 cubicBezierPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    Vector3 point = p0 * uuu;
    point += p1 * (3.0f * uu * t);
    point += p2 * (3.0f * u * tt);
    point += p3 * ttt;
    return point;
}

std::vector<Vector3> sampleBezierPath(const std::vector<Vector3>& controlPoints, int samplesPerSegment)
{
    if (controlPoints.size() < 2)
        return controlPoints;

    samplesPerSegment = std::max(1, samplesPerSegment);

    if (controlPoints.size() >= 4) {
        std::size_t segments = (controlPoints.size() - 1) / 3;
        if (segments > 0) {
            std::vector<Vector3> result;
            result.reserve(segments * static_cast<std::size_t>(samplesPerSegment) + 1);
            result.push_back(controlPoints.front());
            for (std::size_t seg = 0; seg < segments; ++seg) {
                std::size_t base = seg * 3;
                std::size_t endIndex = std::min<std::size_t>(base + 3, controlPoints.size() - 1);
                const Vector3& p0 = controlPoints[base];
                const Vector3& p1 = controlPoints[base + 1];
                const Vector3& p2 = controlPoints[base + 2];
                const Vector3& p3 = controlPoints[endIndex];
                for (int step = 1; step <= samplesPerSegment; ++step) {
                    float t = static_cast<float>(step) / static_cast<float>(samplesPerSegment);
                    result.push_back(cubicBezierPoint(p0, p1, p2, p3, t));
                }
            }

            if (segments * 3 + 1 < controlPoints.size()) {
                for (std::size_t i = segments * 3 + 1; i < controlPoints.size(); ++i)
                    result.push_back(controlPoints[i]);
            }

            return result;
        }
    }

    std::vector<Vector3> sampled;
    sampled.reserve((controlPoints.size() - 1) * static_cast<std::size_t>(samplesPerSegment));
    for (std::size_t i = 0; i + 1 < controlPoints.size(); ++i) {
        const Vector3& a = controlPoints[i];
        const Vector3& b = controlPoints[i + 1];
        sampled.push_back(a);
        for (int step = 1; step < samplesPerSegment; ++step) {
            float t = static_cast<float>(step) / static_cast<float>(samplesPerSegment);
            sampled.push_back(a * (1.0f - t) + b * t);
        }
    }
    sampled.push_back(controlPoints.back());
    return sampled;
}

float polygonArea(const std::vector<int>& loop, const std::vector<HalfEdgeVertex>& verts)
{
    if (loop.size() < 3)
        return 0.0f;
    Vector3 origin = verts[static_cast<std::size_t>(loop[0])].position;
    Vector3 accum(0.0f, 0.0f, 0.0f);
    for (std::size_t i = 1; i + 1 < loop.size(); ++i) {
        Vector3 a = verts[static_cast<std::size_t>(loop[i])].position - origin;
        Vector3 b = verts[static_cast<std::size_t>(loop[i + 1])].position - origin;
        accum += a.cross(b);
    }
    return accum.length() * 0.5f;
}

void alignLoopOrder(const std::vector<Vector3>& reference, std::vector<Vector3>& candidate)
{
    if (reference.size() != candidate.size() || reference.size() < 3)
        return;
    std::size_t count = reference.size();
    float bestScore = std::numeric_limits<float>::max();
    std::size_t bestOffset = 0;
    bool bestReversed = false;

    auto scoreFor = [&](const std::vector<Vector3>& loop, std::size_t offset) {
        float score = 0.0f;
        for (std::size_t i = 0; i < count; ++i) {
            const Vector3& ref = reference[i];
            const Vector3& cand = loop[(i + offset) % count];
            score += (ref - cand).lengthSquared();
        }
        return score;
    };

    std::vector<Vector3> reversed(candidate.rbegin(), candidate.rend());
    for (int mode = 0; mode < 2; ++mode) {
        const std::vector<Vector3>& loop = mode == 0 ? candidate : reversed;
        for (std::size_t offset = 0; offset < count; ++offset) {
            float score = scoreFor(loop, offset);
            if (score < bestScore) {
                bestScore = score;
                bestOffset = offset;
                bestReversed = (mode == 1);
            }
        }
    }

    if (bestReversed)
        std::reverse(candidate.begin(), candidate.end());
    if (bestOffset == 0)
        return;
    std::rotate(candidate.begin(), candidate.begin() + static_cast<std::ptrdiff_t>(bestOffset), candidate.end());
}

std::vector<Vector3> buildRoundedLoop(const std::vector<Vector3>& loop, const RoundCornerOptions& options,
    std::vector<bool>& hardnessOut)
{
    hardnessOut.clear();
    if (loop.size() < 3) {
        hardnessOut.assign(loop.size(), options.tagHardEdges);
        return loop;
    }

    struct Settings {
        float radius = 0.0f;
        int segments = 1;
        CornerStyle style = CornerStyle::Fillet;
        bool hardEdge = false;
    };

    std::unordered_map<std::size_t, CornerOverride> overrideLookup;
    for (const auto& overrideEntry : options.overrides) {
        overrideLookup[overrideEntry.cornerIndex] = overrideEntry;
    }

    auto resolveSettings = [&](std::size_t index) {
        Settings s;
        s.radius = options.radius;
        s.segments = std::max(1, options.segments);
        s.style = options.style;
        s.hardEdge = options.tagHardEdges;
        auto it = overrideLookup.find(index);
        if (it != overrideLookup.end()) {
            if (it->second.radius >= 0.0f)
                s.radius = it->second.radius;
            if (it->second.segments > 0)
                s.segments = it->second.segments;
            s.style = it->second.style;
            if (it->second.hardEdge.has_value())
                s.hardEdge = *it->second.hardEdge;
        }
        return s;
    };

    std::vector<Vector3> result;
    result.reserve(loop.size() * 6);
    hardnessOut.reserve(loop.size() * 6);

    for (std::size_t i = 0; i < loop.size(); ++i) {
        const Vector3& current = loop[i];
        const Vector3& prev = loop[(i + loop.size() - 1) % loop.size()];
        const Vector3& next = loop[(i + 1) % loop.size()];
        Vector3 toPrev = prev - current;
        Vector3 toNext = next - current;
        float lenPrev = toPrev.length();
        float lenNext = toNext.length();
        if (lenPrev <= kEpsilon || lenNext <= kEpsilon) {
            if (result.empty() || (result.back() - current).length() > kEpsilon) {
                result.push_back(current);
                hardnessOut.push_back(options.tagHardEdges);
            }
            continue;
        }

        Settings settings = resolveSettings(i);
        Vector3 dirPrev = toPrev / lenPrev;
        Vector3 dirNext = toNext / lenNext;
        Vector3 outgoingPrev = -dirPrev;
        Vector3 outgoingNext = dirNext;
        float cosTheta = std::clamp(outgoingPrev.dot(outgoingNext), -1.0f, 1.0f);
        float theta = std::acos(cosTheta);
        if (theta <= kEpsilon) {
            // Degenerate; keep original point.
            if (result.empty() || (result.back() - current).length() > kEpsilon) {
                result.push_back(current);
                hardnessOut.push_back(settings.hardEdge);
            }
            continue;
        }

        float tanHalf = std::tan(theta * 0.5f);
        float sinHalf = std::sin(theta * 0.5f);
        if (std::fabs(tanHalf) <= kEpsilon || std::fabs(sinHalf) <= kEpsilon) {
            if (result.empty() || (result.back() - current).length() > kEpsilon) {
                result.push_back(current);
                hardnessOut.push_back(settings.hardEdge);
            }
            continue;
        }

        float maxRadiusPrev = lenPrev * tanHalf;
        float maxRadiusNext = lenNext * tanHalf;
        float radius = std::min(settings.radius, std::min(maxRadiusPrev, maxRadiusNext));
        if (radius <= kEpsilon) {
            if (result.empty() || (result.back() - current).length() > kEpsilon) {
                result.push_back(current);
                hardnessOut.push_back(settings.hardEdge);
            }
            continue;
        }

        float tangentDistance = radius / tanHalf;
        tangentDistance = std::min({ tangentDistance, lenPrev - kEpsilon, lenNext - kEpsilon });
        tangentDistance = std::max(tangentDistance, radius * 0.2f);

        Vector3 startPoint = current + dirPrev * tangentDistance;
        Vector3 endPoint = current + dirNext * tangentDistance;

        if (settings.style == CornerStyle::Chamfer) {
            if (result.empty() || (result.back() - startPoint).length() > kEpsilon) {
                result.push_back(startPoint);
                hardnessOut.push_back(settings.hardEdge);
            }
            for (int s = 1; s < settings.segments; ++s) {
                float t = static_cast<float>(s) / static_cast<float>(settings.segments);
                Vector3 mid = startPoint * (1.0f - t) + endPoint * t;
                result.push_back(mid);
                hardnessOut.push_back(settings.hardEdge);
            }
            result.push_back(endPoint);
            hardnessOut.push_back(settings.hardEdge);
            continue;
        }

        Vector3 bisector = (outgoingPrev + outgoingNext).normalized();
        if (bisector.lengthSquared() <= kEpsilon) {
            // Straight line; behave as chamfer
            if (result.empty() || (result.back() - startPoint).length() > kEpsilon) {
                result.push_back(startPoint);
                hardnessOut.push_back(settings.hardEdge);
            }
            for (int s = 1; s < settings.segments; ++s) {
                float t = static_cast<float>(s) / static_cast<float>(settings.segments);
                Vector3 mid = startPoint * (1.0f - t) + endPoint * t;
                result.push_back(mid);
                hardnessOut.push_back(settings.hardEdge);
            }
            result.push_back(endPoint);
            hardnessOut.push_back(settings.hardEdge);
            continue;
        }

        float centerDistance = radius / sinHalf;
        Vector3 center = current + bisector * centerDistance;
        Vector3 startVec = startPoint - center;
        Vector3 endVec = endPoint - center;
        Vector3 normal = startVec.cross(endVec);
        if (normal.lengthSquared() <= kEpsilon) {
            normal = dirPrev.cross(dirNext);
        }
        if (normal.lengthSquared() <= kEpsilon) {
            normal = Vector3(0.0f, 1.0f, 0.0f);
        }
        normal = normal.normalized();

        float arcAngle = std::acos(std::clamp(startVec.normalized().dot(endVec.normalized()), -1.0f, 1.0f));
        if (result.empty() || (result.back() - startPoint).length() > kEpsilon) {
            result.push_back(startPoint);
            hardnessOut.push_back(settings.hardEdge);
        }
        for (int s = 1; s <= settings.segments; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(settings.segments + 1);
            float angle = arcAngle * t;
            Vector3 rotated = GeometryTransforms::rotateAroundAxis(startPoint, center, normal, angle);
            result.push_back(rotated);
            hardnessOut.push_back(settings.hardEdge);
        }
        result.push_back(endPoint);
        hardnessOut.push_back(settings.hardEdge);
    }

    if (result.size() < 3) {
        hardnessOut.assign(loop.size(), options.tagHardEdges);
        return loop;
    }

    if ((result.front() - result.back()).length() > kEpsilon) {
        result.push_back(result.front());
        hardnessOut.push_back(hardnessOut.empty() ? options.tagHardEdges : hardnessOut.front());
    }

    return result;
}

std::vector<Vector3> densifyPath(const std::vector<Vector3>& path, float spacing)
{
    if (path.size() < 2)
        return path;
    spacing = std::max(spacing, 1e-3f);
    std::vector<Vector3> result;
    result.reserve(path.size() * 2);
    result.push_back(path.front());
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        Vector3 a = path[i];
        Vector3 b = path[i + 1];
        float segmentLength = (b - a).length();
        if (segmentLength <= spacing) {
            if ((result.back() - b).length() > kEpsilon) {
                result.push_back(b);
            }
            continue;
        }
        int count = static_cast<int>(std::floor(segmentLength / spacing));
        float step = 1.0f / static_cast<float>(count + 1);
        for (int c = 1; c <= count; ++c) {
            float t = step * static_cast<float>(c);
            Vector3 p = a * (1.0f - t) + b * t;
            if ((result.back() - p).length() > kEpsilon) {
                result.push_back(p);
            }
        }
        if ((result.back() - b).length() > kEpsilon) {
            result.push_back(b);
        }
    }
    return result;
}

std::vector<Vector3> computeVertexNormals(const HalfEdgeMesh& mesh)
{
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();
    const auto& halfEdges = mesh.getHalfEdges();
    std::vector<Vector3> normals(vertices.size(), Vector3(0.0f, 0.0f, 0.0f));
    for (const auto& face : faces) {
        if (face.halfEdge < 0)
            continue;
        Vector3 normal = face.normal;
        if (normal.lengthSquared() <= kEpsilon)
            continue;
        int start = face.halfEdge;
        int current = start;
        do {
            if (current < 0 || current >= static_cast<int>(halfEdges.size()))
                break;
            const auto& he = halfEdges[current];
            if (he.origin >= 0 && he.origin < static_cast<int>(normals.size())) {
                normals[he.origin] += normal;
            }
            current = he.next;
        } while (current != start && current >= 0);
    }
    for (auto& n : normals) {
        float len = n.length();
        if (len > kEpsilon) {
            n /= len;
        }
    }
    return normals;
}

std::vector<std::vector<int>> buildAdjacency(const HalfEdgeMesh& mesh)
{
    const auto& vertices = mesh.getVertices();
    std::vector<std::vector<int>> adjacency(vertices.size());
    const auto& halfEdges = mesh.getHalfEdges();
    for (const auto& he : halfEdges) {
        if (he.origin >= 0 && he.destination >= 0) {
            adjacency[he.origin].push_back(he.destination);
        }
    }
    for (auto& neighbors : adjacency) {
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }
    return adjacency;
}

std::vector<std::vector<int>> extractFaceLoops(const HalfEdgeMesh& mesh)
{
    std::vector<std::vector<int>> loops;
    const auto& faces = mesh.getFaces();
    const auto& halfEdges = mesh.getHalfEdges();
    for (const auto& face : faces) {
        if (face.halfEdge < 0)
            continue;
        std::vector<int> loop;
        int start = face.halfEdge;
        int current = start;
        do {
            if (current < 0 || current >= static_cast<int>(halfEdges.size())) {
                loop.clear();
                break;
            }
            const auto& he = halfEdges[current];
            loop.push_back(he.origin);
            current = he.next;
        } while (current != start && current >= 0);
        if (loop.size() >= 3)
            loops.push_back(std::move(loop));
    }
    return loops;
}

void updateSolidMetadata(Solid& solid)
{
    const auto& mesh = solid.getMesh();
    const auto& verts = mesh.getVertices();
    if (verts.empty())
        return;
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (const auto& v : verts) {
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
    }
    std::vector<Vector3> base;
    for (const auto& v : verts) {
        if (std::fabs(v.position.y - minY) <= 1e-3f) {
            base.push_back(Vector3(v.position.x, minY, v.position.z));
        }
    }
    if (base.size() < 3) {
        base.clear();
        for (const auto& v : verts) {
            base.push_back(Vector3(v.position.x, minY, v.position.z));
        }
    }
    float height = std::max(0.0f, maxY - minY);
    solid.setBaseMetadata(base, height);
}

HalfEdgeMesh buildSkinMesh(const std::vector<std::vector<Vector3>>& loops, bool closeRails, bool capStart, bool capEnd)
{
    HalfEdgeMesh mesh;
    if (loops.size() < 2)
        return mesh;
    std::size_t ringSize = loops.front().size();
    if (ringSize < 3)
        return mesh;
    for (const auto& loop : loops) {
        if (loop.size() != ringSize)
            return mesh;
    }

    std::vector<std::vector<int>> indices(loops.size());
    for (std::size_t i = 0; i < loops.size(); ++i) {
        indices[i].reserve(ringSize);
        for (const auto& point : loops[i]) {
            indices[i].push_back(mesh.addVertex(point));
        }
    }

    if (capStart) {
        std::vector<int> start = indices.front();
        std::reverse(start.begin(), start.end());
        mesh.addFace(start);
    }
    if (capEnd) {
        mesh.addFace(indices.back());
    }

    for (std::size_t ring = 0; ring + 1 < indices.size(); ++ring) {
        for (std::size_t v = 0; v < ringSize; ++v) {
            std::size_t next = (v + 1) % ringSize;
            std::vector<int> quad = {
                indices[ring][v],
                indices[ring][next],
                indices[ring + 1][next],
                indices[ring + 1][v]
            };
            mesh.addFace(quad);
        }
    }

    if (closeRails && indices.size() > 2) {
        std::size_t last = indices.size() - 1;
        for (std::size_t v = 0; v < ringSize; ++v) {
            std::size_t next = (v + 1) % ringSize;
            std::vector<int> quad = {
                indices[last][v],
                indices[last][next],
                indices[0][next],
                indices[0][v]
            };
            mesh.addFace(quad);
        }
    }

    mesh.heal(kEpsilon, kEpsilon);
    mesh.recomputeNormals();
    return mesh;
}

void laplacianSmooth(HalfEdgeMesh& mesh, int passes, float alpha)
{
    passes = std::max(0, passes);
    if (passes == 0)
        return;
    alpha = std::clamp(alpha, 0.01f, 0.9f);
    for (int pass = 0; pass < passes; ++pass) {
        auto adjacency = buildAdjacency(mesh);
        const auto original = mesh.getVertices();
        auto& verts = mesh.getVertices();
        if (adjacency.size() != verts.size())
            break;
        for (std::size_t i = 0; i < verts.size(); ++i) {
            const auto& neighbors = adjacency[i];
            if (neighbors.empty())
                continue;
            Vector3 average(0.0f, 0.0f, 0.0f);
            for (int n : neighbors) {
                if (n >= 0 && n < static_cast<int>(verts.size()))
                    average += original[static_cast<std::size_t>(n)].position;
            }
            average /= static_cast<float>(neighbors.size());
            verts[i].position = verts[i].position * (1.0f - alpha) + average * alpha;
        }
    }
    mesh.recomputeNormals();
}

HalfEdgeMesh buildLoftMesh(const std::vector<Vector3>& bottom, const std::vector<Vector3>& top)
{
    std::vector<std::vector<Vector3>> loops;
    loops.push_back(bottom);
    loops.push_back(top);
    return buildSkinMesh(loops, false, true, true);
}

bool connectRings(HalfEdgeMesh& mesh, const std::vector<int>& a, const std::vector<int>& b)
{
    if (a.size() < 2 || a.size() != b.size())
        return false;
    bool ok = true;
    for (std::size_t i = 0; i < a.size(); ++i) {
        std::size_t next = (i + 1) % a.size();
        std::vector<int> quad = { a[i], a[next], b[next], b[i] };
        if (mesh.addFace(quad) < 0) {
            ok = false;
        }
    }
    return ok;
}

bool addLoopFace(HalfEdgeMesh& mesh, const std::vector<int>& loop, bool reverse)
{
    if (loop.size() < 3)
        return false;
    if (!reverse)
        return mesh.addFace(loop) >= 0;
    std::vector<int> reversed(loop.rbegin(), loop.rend());
    return mesh.addFace(reversed) >= 0;
}

} // namespace

RoundCorner::RoundCorner(GeometryKernel& kernel)
    : geometry(kernel)
{
}

bool RoundCorner::filletCurve(Curve& curve, const RoundCornerOptions& options) const
{
    auto loop = curve.getBoundaryLoop();
    if (loop.size() < 3)
        return false;
    std::vector<bool> hardness;
    auto rounded = buildRoundedLoop(loop, options, hardness);
    if (rounded.size() < 3)
        return false;
    return curve.rebuildFromPoints(rounded, hardness);
}

std::unique_ptr<Curve> RoundCorner::createFilleted(const Curve& curve, const RoundCornerOptions& options) const
{
    auto loop = curve.getBoundaryLoop();
    if (loop.size() < 3)
        return nullptr;
    std::vector<bool> hardness;
    auto rounded = buildRoundedLoop(loop, options, hardness);
    return Curve::createFromPoints(rounded, hardness);
}

CurveIt::CurveIt(GeometryKernel& kernel)
    : geometry(kernel)
{
}

Solid* CurveIt::loft(const Curve& start, const Curve& end, const LoftOptions& options) const
{
    auto baseLoop = start.getBoundaryLoop();
    auto topLoop = end.getBoundaryLoop();
    if (baseLoop.size() < 3 || topLoop.size() < 3)
        return nullptr;
    int sectionCount = std::max(2, options.sections);
    std::size_t vertexCount = std::max<std::size_t>({ baseLoop.size(), topLoop.size(), static_cast<std::size_t>(3) });
    auto baseSamples = resampleLoop(baseLoop, vertexCount);
    auto topSamples = resampleLoop(topLoop, vertexCount);
    if (baseSamples.size() != topSamples.size())
        return nullptr;

    if (options.symmetricPairing)
        alignLoopOrder(baseSamples, topSamples);

    Vector3 baseCentroid = computeCentroid(baseSamples);
    Vector3 topCentroid = computeCentroid(topSamples);
    Vector3 baseNormal = MeshUtils::computePolygonNormal(baseSamples);
    if (baseNormal.lengthSquared() <= kEpsilon)
        baseNormal = Vector3(0.0f, 1.0f, 0.0f);
    Vector3 topNormal = MeshUtils::computePolygonNormal(topSamples);
    if (topNormal.lengthSquared() <= kEpsilon)
        topNormal = baseNormal;
    Vector3 axis = topNormal.cross(baseNormal);
    if (axis.lengthSquared() > kEpsilon) {
        axis = axis.normalized();
        float angle = std::acos(std::clamp(topNormal.normalized().dot(baseNormal.normalized()), -1.0f, 1.0f));
        for (auto& point : topSamples) {
            point = GeometryTransforms::rotateAroundAxis(point, topCentroid, axis, angle);
        }
    }

    std::size_t sectionSamples = static_cast<std::size_t>(sectionCount);
    std::vector<Vector3> centroidPath;
    if (!options.railA.empty())
        centroidPath = resamplePath(options.railA, sectionSamples);
    if (centroidPath.size() != sectionSamples) {
        centroidPath.resize(sectionSamples);
        for (int s = 0; s < sectionCount; ++s) {
            float t = sectionCount == 1 ? 0.0f : static_cast<float>(s) / static_cast<float>(sectionCount - 1);
            centroidPath[static_cast<std::size_t>(s)] = baseCentroid * (1.0f - t) + topCentroid * t;
        }
    }

    std::vector<Vector3> orientationPath;
    if (!options.railB.empty())
        orientationPath = resamplePath(options.railB, sectionSamples);

    std::vector<std::vector<Vector3>> sections;
    sections.reserve(sectionCount);
    constexpr float degToRad = 3.14159265358979323846f / 180.0f;
    for (int s = 0; s < sectionCount; ++s) {
        float t = sectionCount == 1 ? 0.0f : static_cast<float>(s) / static_cast<float>(sectionCount - 1);
        std::size_t idx = std::min<std::size_t>(static_cast<std::size_t>(s), centroidPath.size() - 1);
        Vector3 origin = centroidPath[idx];
        Vector3 tangent = baseNormal.normalized();
        if (sectionCount > 1) {
            Vector3 forward(0.0f, 0.0f, 0.0f);
            if (s + 1 < sectionCount) {
                std::size_t nextIndex = std::min<std::size_t>(static_cast<std::size_t>(s + 1), centroidPath.size() - 1);
                forward = centroidPath[nextIndex] - origin;
            }
            if (forward.lengthSquared() <= kEpsilon && s > 0) {
                std::size_t prevIndex = std::min<std::size_t>(static_cast<std::size_t>(s - 1), centroidPath.size() - 1);
                forward = origin - centroidPath[prevIndex];
            }
            if (forward.lengthSquared() > kEpsilon)
                tangent = forward.normalized();
        }
        Vector3 up = baseNormal.normalized();
        if (!orientationPath.empty()) {
            std::size_t orientIndex = std::min<std::size_t>(static_cast<std::size_t>(s), orientationPath.size() - 1);
            Vector3 orient = orientationPath[orientIndex] - origin;
            if (orient.lengthSquared() > kEpsilon)
                up = orient.normalized();
        }
        if (up.lengthSquared() <= kEpsilon)
            up = baseNormal.normalized();
        Vector3 right = tangent.cross(up);
        if (right.lengthSquared() <= kEpsilon)
            right = perpendicularVector(tangent);
        else
            right = right.normalized();
        up = right.cross(tangent).normalized();
        if (tangent.lengthSquared() <= kEpsilon)
            tangent = baseNormal.normalized();

        float twist = options.twistDegrees * t * degToRad;

        std::vector<Vector3> loop;
        loop.reserve(vertexCount);
        for (std::size_t v = 0; v < vertexCount; ++v) {
            Vector3 basePoint = baseSamples[v];
            Vector3 topPoint = topSamples[v];
            Vector3 blended = basePoint * (1.0f - t) + topPoint * t;
            Vector3 centroidBlend = baseCentroid * (1.0f - t) + topCentroid * t;
            Vector3 translated = blended + (origin - centroidBlend);
            Vector3 rotated = GeometryTransforms::rotateAroundAxis(translated, origin, tangent, twist);
            // keep points aligned with up/right plane by projecting components
            Vector3 relative = rotated - origin;
            float alongUp = relative.dot(up);
            float alongRight = relative.dot(right);
            float alongTangent = relative.dot(tangent);
            Vector3 reconstructed = origin + right * alongRight + up * alongUp + tangent * alongTangent;
            loop.push_back(reconstructed);
        }
        sections.push_back(std::move(loop));
    }

    HalfEdgeMesh mesh = buildSkinMesh(sections, options.closeRails, true, true);
    if (mesh.getVertices().empty())
        return nullptr;
    if (options.smoothingPasses > 0)
        laplacianSmooth(mesh, options.smoothingPasses, 0.25f);
    if (options.smoothNormals)
        mesh.recomputeNormals();

    auto solidPtr = Solid::createFromMesh(std::move(mesh));
    if (!solidPtr)
        return nullptr;
    Solid* raw = static_cast<Solid*>(geometry.addObject(std::move(solidPtr)));
    if (!raw)
        return nullptr;
    updateSolidMetadata(*raw);
    return raw;
}

bool PushAndPull::thicken(Solid& solid, const PushPullOptions& options) const
{
    auto& mesh = solid.getMesh();
    if (mesh.getVertices().empty())
        return false;
    float distance = std::clamp(options.distance, -options.maxThickness, options.maxThickness);
    if (std::fabs(distance) <= kEpsilon)
        return false;

    mesh.recomputeNormals();
    auto normals = computeVertexNormals(mesh);
    if (normals.size() != mesh.getVertices().size())
        return false;

    if (!options.generateInnerShell) {
        auto& verts = mesh.getVertices();
        for (std::size_t i = 0; i < verts.size(); ++i)
            verts[i].position += normals[i] * distance;
        mesh.recomputeNormals();
        if (options.createCaps)
            mesh.heal(kEpsilon, kEpsilon);
        updateSolidMetadata(solid);
        return true;
    }

    HalfEdgeMesh newMesh;
    const auto& originalVerts = mesh.getVertices();
    std::vector<int> innerIndices(originalVerts.size(), -1);
    std::vector<int> outerIndices(originalVerts.size(), -1);
    for (std::size_t i = 0; i < originalVerts.size(); ++i) {
        innerIndices[i] = newMesh.addVertex(originalVerts[i].position);
        Vector3 offset = normals[i] * distance;
        outerIndices[i] = newMesh.addVertex(originalVerts[i].position + offset);
    }

    auto faceLoops = extractFaceLoops(mesh);
    for (const auto& loop : faceLoops) {
        std::vector<int> innerLoop;
        std::vector<int> outerLoop;
        innerLoop.reserve(loop.size());
        outerLoop.reserve(loop.size());
        for (int index : loop) {
            if (index < 0 || static_cast<std::size_t>(index) >= innerIndices.size())
                continue;
            innerLoop.push_back(innerIndices[static_cast<std::size_t>(index)]);
            outerLoop.push_back(outerIndices[static_cast<std::size_t>(index)]);
        }
        if (innerLoop.size() < 3)
            continue;
        if (options.createCaps) {
            std::vector<int> reversed = innerLoop;
            std::reverse(reversed.begin(), reversed.end());
            newMesh.addFace(reversed);
            newMesh.addFace(outerLoop);
        }
    }

    std::unordered_set<long long> processedEdges;
    for (const auto& loop : faceLoops) {
        for (std::size_t i = 0; i < loop.size(); ++i) {
            int v0 = loop[i];
            int v1 = loop[(i + 1) % loop.size()];
            if (v0 < 0 || v1 < 0)
                continue;
            if (static_cast<std::size_t>(v0) >= innerIndices.size() || static_cast<std::size_t>(v1) >= innerIndices.size())
                continue;
            long long key = makeUndirectedEdge(v0, v1);
            if (processedEdges.insert(key).second) {
                std::vector<int> quad = {
                    innerIndices[static_cast<std::size_t>(v0)],
                    innerIndices[static_cast<std::size_t>(v1)],
                    outerIndices[static_cast<std::size_t>(v1)],
                    outerIndices[static_cast<std::size_t>(v0)]
                };
                newMesh.addFace(quad);
            }
        }
    }

    newMesh.heal(kEpsilon, kEpsilon);
    newMesh.recomputeNormals();
    solid.setMesh(std::move(newMesh));
    updateSolidMetadata(solid);
    return true;
}

Surface::Surface(GeometryKernel& kernel)
    : geometry(kernel)
{
}

Curve* Surface::drawPolylineOnSolid(const Solid& solid, const std::vector<Vector3>& path, const SurfaceDrawOptions& options) const
{
    if (path.size() < 2)
        return nullptr;
    const auto& mesh = solid.getMesh();
    const auto& verts = mesh.getVertices();
    if (verts.empty())
        return nullptr;
    auto dense = densifyPath(path, options.samplingDistance);
    std::vector<Vector3> projected;
    projected.reserve(dense.size());
    const auto& triangles = mesh.getTriangles();
    for (const auto& point : dense) {
        float best = std::numeric_limits<float>::max();
        Vector3 bestPoint = point;
        Vector3 bestNormal(0.0f, 1.0f, 0.0f);
        for (const auto& tri : triangles) {
            if (tri.v0 < 0 || tri.v1 < 0 || tri.v2 < 0)
                continue;
            const Vector3& a = verts[static_cast<std::size_t>(tri.v0)].position;
            const Vector3& b = verts[static_cast<std::size_t>(tri.v1)].position;
            const Vector3& c = verts[static_cast<std::size_t>(tri.v2)].position;
            Vector3 candidate = closestPointOnTriangle(point, a, b, c);
            float dist = (candidate - point).lengthSquared();
            if (dist < best) {
                best = dist;
                bestPoint = candidate;
                bestNormal = tri.normal.lengthSquared() > kEpsilon ? tri.normal.normalized() : (b - a).cross(c - a).normalized();
            }
        }
        if (best == std::numeric_limits<float>::max()) {
            projected.push_back(point);
        } else {
            projected.push_back(bestPoint + bestNormal * options.projectionOffset);
        }
    }

    if (options.remesh) {
        projected = MeshUtils::weldSequential(projected, options.samplingDistance * 0.5f);
        projected = MeshUtils::collapseTinyEdges(projected, options.samplingDistance * 0.5f);
    }

    if ((projected.front() - projected.back()).length() > kEpsilon) {
        projected.push_back(projected.front());
    }

    GeometryObject* object = geometry.addCurve(projected);
    if (!object || object->getType() != GeometryObject::ObjectType::Curve)
        return nullptr;
    return static_cast<Curve*>(object);
}

BezierKnife::BezierKnife(GeometryKernel& kernel)
    : geometry(kernel)
{
}

Curve* BezierKnife::cut(const Solid& solid, const std::vector<Vector3>& controlPoints, const KnifeOptions& options) const
{
    if (controlPoints.size() < 2)
        return nullptr;
    Solid& writableSolid = const_cast<Solid&>(solid);
    auto& mesh = writableSolid.getMesh();
    if (mesh.getVertices().empty())
        return nullptr;
    std::vector<Vector3> sampled = sampleBezierPath(controlPoints, options.samplesPerSegment);
    if (sampled.size() < 2)
        return nullptr;

    const auto& triangles = mesh.getTriangles();
    const auto& vertsRef = mesh.getVertices();
    struct ProjectedSample {
        Vector3 position;
        Vector3 normal;
    };
    std::vector<ProjectedSample> projected;
    projected.reserve(sampled.size());
    for (const auto& pt : sampled) {
        float best = std::numeric_limits<float>::max();
        Vector3 bestPoint = pt;
        Vector3 bestNormal(0.0f, 1.0f, 0.0f);
        for (const auto& tri : triangles) {
            if (tri.v0 < 0 || tri.v1 < 0 || tri.v2 < 0)
                continue;
            const Vector3& a = vertsRef[static_cast<std::size_t>(tri.v0)].position;
            const Vector3& b = vertsRef[static_cast<std::size_t>(tri.v1)].position;
            const Vector3& c = vertsRef[static_cast<std::size_t>(tri.v2)].position;
            Vector3 candidate = closestPointOnTriangle(pt, a, b, c);
            float dist = (candidate - pt).lengthSquared();
            if (dist < best) {
                best = dist;
                bestPoint = candidate;
                bestNormal = tri.normal.lengthSquared() > kEpsilon ? tri.normal.normalized() : (b - a).cross(c - a).normalized();
            }
        }
        if (best == std::numeric_limits<float>::max())
            bestPoint = pt;
        if (bestNormal.lengthSquared() <= kEpsilon)
            bestNormal = Vector3(0.0f, 1.0f, 0.0f);
        projected.push_back({ bestPoint, bestNormal });
    }

    std::vector<Vector3> projectedPositions;
    projectedPositions.reserve(projected.size());
    for (const auto& entry : projected)
        projectedPositions.push_back(entry.position);

    if ((projectedPositions.front() - projectedPositions.back()).length() > kEpsilon)
        projectedPositions.push_back(projectedPositions.front());

    if (options.removeInterior) {
        auto faceLoops = extractFaceLoops(mesh);
        const auto& faces = mesh.getFaces();
        const auto& vertsBefore = mesh.getVertices();
        HalfEdgeMesh rebuilt;
        std::vector<int> remap(vertsBefore.size(), -1);

        auto mapVertex = [&](int index) {
            if (index < 0)
                return -1;
            auto idx = static_cast<std::size_t>(index);
            if (idx >= remap.size())
                return -1;
            if (remap[idx] == -1)
                remap[idx] = rebuilt.addVertex(vertsBefore[idx].position);
            return remap[idx];
        };

        for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
            if (faceIndex >= faceLoops.size())
                break;
            const auto& loop = faceLoops[faceIndex];
            Vector3 centroid(0.0f, 0.0f, 0.0f);
            for (int vIdx : loop)
                centroid += vertsBefore[static_cast<std::size_t>(vIdx)].position;
            centroid /= static_cast<float>(loop.size());
            float dist = distanceToPolyline(centroid, projectedPositions);
            if (dist <= options.cutWidth)
                continue;
            std::vector<int> remappedLoop;
            remappedLoop.reserve(loop.size());
            for (int vIdx : loop) {
                int mapped = mapVertex(vIdx);
                if (mapped >= 0)
                    remappedLoop.push_back(mapped);
            }
            if (remappedLoop.size() >= 3)
                rebuilt.addFace(remappedLoop);
        }

        rebuilt.heal(kEpsilon, kEpsilon);
        rebuilt.recomputeNormals();
        writableSolid.setMesh(std::move(rebuilt));
    }

    if (std::fabs(options.extrusionHeight) > kEpsilon) {
        auto normals = computeVertexNormals(mesh);
        auto& vertsAfter = mesh.getVertices();
        if (normals.size() == vertsAfter.size()) {
            for (std::size_t i = 0; i < vertsAfter.size(); ++i) {
                float dist = distanceToPolyline(vertsAfter[i].position, projectedPositions);
                if (dist <= options.cutWidth + 1e-4f)
                    vertsAfter[i].position -= normals[i] * options.extrusionHeight;
            }
            mesh.heal(kEpsilon, kEpsilon);
            mesh.recomputeNormals();
        }
    }

    std::vector<Vector3> imprint = MeshUtils::weldSequential(projectedPositions, options.cutWidth * 0.5f);
    imprint = MeshUtils::collapseTinyEdges(imprint, options.cutWidth * 0.5f);
    if (!imprint.empty() && (imprint.front() - imprint.back()).length() > kEpsilon)
        imprint.push_back(imprint.front());

    GeometryObject* object = geometry.addCurve(imprint);
    if (!object || object->getType() != GeometryObject::ObjectType::Curve)
        return nullptr;
    updateSolidMetadata(writableSolid);
    return static_cast<Curve*>(object);
}

bool QuadTools::retopologizeToQuads(Solid& solid, const QuadConversionOptions& options) const
{
    auto& mesh = solid.getMesh();
    if (mesh.getVertices().empty())
        return false;
    mesh.heal(options.mergeThreshold, options.mergeThreshold);
    mesh.recomputeNormals();

    auto loops = extractFaceLoops(mesh);
    const auto& faces = mesh.getFaces();
    if (loops.size() != faces.size())
        return false;

    const auto& verts = mesh.getVertices();
    HalfEdgeMesh rebuilt;
    std::vector<int> remap(verts.size(), -1);
    auto mapVertex = [&](int index) {
        if (index < 0)
            return -1;
        auto idx = static_cast<std::size_t>(index);
        if (idx >= remap.size())
            return -1;
        if (remap[idx] == -1)
            remap[idx] = rebuilt.addVertex(verts[idx].position);
        return remap[idx];
    };

    std::unordered_map<long long, int> edgeToFace;
    for (std::size_t f = 0; f < loops.size(); ++f) {
        const auto& loop = loops[f];
        for (std::size_t i = 0; i < loop.size(); ++i) {
            int v0 = loop[i];
            int v1 = loop[(i + 1) % loop.size()];
            edgeToFace.emplace(makeUndirectedEdge(v0, v1), static_cast<int>(f));
        }
    }

    std::vector<bool> used(loops.size(), false);
    for (std::size_t f = 0; f < loops.size(); ++f) {
        const auto& loop = loops[f];
        if (loop.size() == 4) {
            std::vector<int> faceLoop;
            faceLoop.reserve(4);
            for (int v : loop) {
                int mapped = mapVertex(v);
                if (mapped >= 0)
                    faceLoop.push_back(mapped);
            }
            if (faceLoop.size() == 4)
                rebuilt.addFace(faceLoop);
            continue;
        }

        if (loop.size() == 3 && !used[f]) {
            bool merged = false;
            for (std::size_t i = 0; i < loop.size(); ++i) {
                int v0 = loop[i];
                int v1 = loop[(i + 1) % loop.size()];
                auto it = edgeToFace.find(makeUndirectedEdge(v0, v1));
                if (it != edgeToFace.end() && static_cast<std::size_t>(it->second) != f) {
                    std::size_t other = static_cast<std::size_t>(it->second);
                    if (used[other])
                        continue;
                    const auto& otherLoop = loops[other];
                    auto otherIt = std::find(otherLoop.begin(), otherLoop.end(), v0);
                    auto otherNext = std::find(otherLoop.begin(), otherLoop.end(), v1);
                    if (otherIt == otherLoop.end() || otherNext == otherLoop.end())
                        continue;
                    int fv = loop[(i + 2) % loop.size()];
                    auto idxB = std::find(otherLoop.begin(), otherLoop.end(), v0);
                    int gv = -1;
                    if (idxB != otherLoop.end()) {
                        std::size_t pos = static_cast<std::size_t>(std::distance(otherLoop.begin(), idxB));
                        gv = otherLoop[(pos + otherLoop.size() - 1) % otherLoop.size()];
                        if (gv == v1)
                            gv = otherLoop[(pos + 1) % otherLoop.size()];
                    }
                    if (gv < 0)
                        continue;
                    std::vector<int> quad = {
                        mapVertex(fv),
                        mapVertex(v0),
                        mapVertex(gv),
                        mapVertex(v1)
                    };
                    if (std::all_of(quad.begin(), quad.end(), [](int idx) { return idx >= 0; })) {
                        rebuilt.addFace(quad);
                        used[f] = true;
                        used[other] = true;
                        merged = true;
                        break;
                    }
                }
            }
            if (merged)
                continue;
        }

        // fallback: split polygon into strips of quads
        if (loop.size() > 4) {
            for (std::size_t i = 0; i + 3 < loop.size(); i += 2) {
                std::vector<int> quad = {
                    mapVertex(loop[i]),
                    mapVertex(loop[(i + 1) % loop.size()]),
                    mapVertex(loop[(i + 2) % loop.size()]),
                    mapVertex(loop[(i + 3) % loop.size()])
                };
                if (std::all_of(quad.begin(), quad.end(), [](int idx) { return idx >= 0; }))
                    rebuilt.addFace(quad);
            }
            continue;
        }
    }

    if (rebuilt.getVertices().empty())
        return false;

    rebuilt.heal(options.mergeThreshold, options.mergeThreshold);
    rebuilt.recomputeNormals();
    if (options.validateTopology) {
        auto quads = extractFaceLoops(rebuilt);
        for (const auto& face : quads) {
            if (face.size() != 4)
                return false;
        }
    }

    solid.setMesh(std::move(rebuilt));
    updateSolidMetadata(solid);
    return true;
}

bool SubD::subdivide(Solid& solid, const SubdivisionOptions& options) const
{
    if (options.levels <= 0)
        return false;
    for (int level = 0; level < options.levels; ++level) {
        auto& mesh = solid.getMesh();
        if (mesh.getVertices().empty())
            return false;

        auto loops = extractFaceLoops(mesh);
        const auto& faces = mesh.getFaces();
        const auto& verts = mesh.getVertices();
        if (loops.size() != faces.size())
            return false;

        std::vector<Vector3> facePoints(loops.size(), Vector3());
        std::vector<std::vector<int>> vertexFaces(verts.size());
        std::vector<std::vector<long long>> vertexEdges(verts.size());

        for (std::size_t f = 0; f < loops.size(); ++f) {
            Vector3 sum(0.0f, 0.0f, 0.0f);
            for (int index : loops[f]) {
                if (index < 0 || static_cast<std::size_t>(index) >= verts.size())
                    continue;
                sum += verts[static_cast<std::size_t>(index)].position;
                vertexFaces[static_cast<std::size_t>(index)].push_back(static_cast<int>(f));
            }
            facePoints[f] = sum / static_cast<float>(loops[f].size());
        }

        std::unordered_map<long long, Vector3> edgePoints;
        std::unordered_map<long long, std::vector<int>> edgeFaces;
        for (std::size_t f = 0; f < loops.size(); ++f) {
            const auto& loop = loops[f];
            for (std::size_t i = 0; i < loop.size(); ++i) {
                int v0 = loop[i];
                int v1 = loop[(i + 1) % loop.size()];
                long long key = makeUndirectedEdge(v0, v1);
                Vector3 midpoint = (verts[static_cast<std::size_t>(v0)].position + verts[static_cast<std::size_t>(v1)].position) * 0.5f;
                edgePoints[key] += midpoint;
                edgeFaces[key].push_back(static_cast<int>(f));
                vertexEdges[static_cast<std::size_t>(v0)].push_back(key);
                vertexEdges[static_cast<std::size_t>(v1)].push_back(key);
            }
        }

        std::unordered_map<long long, int> edgeIndex;
        HalfEdgeMesh refined;
        std::vector<int> vertexIndex(verts.size(), -1);

        // compute face point vertices
        std::vector<int> facePointIndex(facePoints.size(), -1);
        for (std::size_t f = 0; f < facePoints.size(); ++f) {
            facePointIndex[f] = refined.addVertex(facePoints[f]);
        }

        // compute edge points and add to mesh
        for (const auto& entry : edgePoints) {
            long long key = entry.first;
            Vector3 sum = entry.second;
            auto facesForEdge = edgeFaces[key];
            Vector3 faceContribution(0.0f, 0.0f, 0.0f);
            for (int faceId : facesForEdge)
                faceContribution += facePoints[static_cast<std::size_t>(faceId)];
            Vector3 newPos = (sum + faceContribution) / static_cast<float>(facesForEdge.size() + 2);
            edgeIndex[key] = refined.addVertex(newPos);
        }

        // compute updated original vertex positions
        for (std::size_t i = 0; i < verts.size(); ++i) {
            const auto& position = verts[i].position;
            const auto& facesAround = vertexFaces[i];
            const auto& edgesAround = vertexEdges[i];
            if (facesAround.empty() || edgesAround.empty()) {
                vertexIndex[i] = refined.addVertex(position);
                continue;
            }
            Vector3 F(0.0f, 0.0f, 0.0f);
            for (int faceId : facesAround)
                F += facePoints[static_cast<std::size_t>(faceId)];
            F /= static_cast<float>(facesAround.size());

            Vector3 R(0.0f, 0.0f, 0.0f);
            for (long long edgeKeyValue : edgesAround) {
                long long eKey = edgeKeyValue;
                auto it = edgePoints.find(eKey);
                if (it != edgePoints.end())
                    R += it->second / 2.0f; // average of original edge endpoints
            }
            R /= static_cast<float>(edgesAround.size());
            float n = static_cast<float>(facesAround.size());
            Vector3 newPos = (F + R * 2.0f + position * (n - 3.0f)) / n;
            vertexIndex[i] = refined.addVertex(newPos);
        }

        // rebuild faces
        for (std::size_t f = 0; f < loops.size(); ++f) {
            const auto& loop = loops[f];
            std::size_t count = loop.size();
            for (std::size_t i = 0; i < count; ++i) {
                int v0 = loop[i];
                int v1 = loop[(i + 1) % count];
                long long keyCurrent = makeUndirectedEdge(v0, v1);
                long long keyPrev = makeUndirectedEdge(loop[(i + count - 1) % count], v0);
                std::vector<int> quad = {
                    vertexIndex[static_cast<std::size_t>(v0)],
                    edgeIndex[keyCurrent],
                    facePointIndex[f],
                    edgeIndex[keyPrev]
                };
                if (std::all_of(quad.begin(), quad.end(), [](int idx) { return idx >= 0; }))
                    refined.addFace(quad);
            }
        }

        refined.heal(kEpsilon, kEpsilon);
        refined.recomputeNormals();
        solid.setMesh(std::move(refined));
    }

    updateSolidMetadata(solid);
    return true;
}

bool Weld::apply(Solid& solid, const WeldOptions& options) const
{
    auto& mesh = solid.getMesh();
    if (mesh.getVertices().empty())
        return false;
    auto& verts = mesh.getVertices();
    Vector3 direction = options.direction.lengthSquared() > kEpsilon ? options.direction.normalized() : Vector3(0.0f, 1.0f, 0.0f);
    float directionalWindow = std::max(0.0f, options.directionalWeight) * options.tolerance * 2.0f;

    if (directionalWindow <= kEpsilon) {
        mesh.heal(options.tolerance, options.tolerance);
        mesh.recomputeNormals();
        updateSolidMetadata(solid);
        return true;
    }

    std::vector<int> parent(verts.size());
    std::vector<int> rank(verts.size(), 0);
    std::vector<float> projection(verts.size(), 0.0f);
    for (std::size_t i = 0; i < verts.size(); ++i) {
        parent[i] = static_cast<int>(i);
        projection[i] = verts[i].position.dot(direction);
    }

    auto find = [&](int x) {
        while (parent[x] != x)
            x = parent[x] = parent[parent[x]];
        return x;
    };

    auto unite = [&](int a, int b) {
        int rootA = find(a);
        int rootB = find(b);
        if (rootA == rootB)
            return;
        if (rank[rootA] < rank[rootB])
            parent[rootA] = rootB;
        else if (rank[rootA] > rank[rootB])
            parent[rootB] = rootA;
        else {
            parent[rootB] = rootA;
            ++rank[rootA];
        }
    };

    for (std::size_t i = 0; i < verts.size(); ++i) {
        for (std::size_t j = i + 1; j < verts.size(); ++j) {
            Vector3 delta = verts[i].position - verts[j].position;
            if (delta.length() > options.tolerance)
                continue;
            if (std::fabs(projection[i] - projection[j]) > directionalWindow)
                continue;
            unite(static_cast<int>(i), static_cast<int>(j));
        }
    }

    std::unordered_map<int, std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < verts.size(); ++i)
        groups[find(static_cast<int>(i))].push_back(i);

    for (const auto& entry : groups) {
        Vector3 average(0.0f, 0.0f, 0.0f);
        for (std::size_t idx : entry.second)
            average += verts[idx].position;
        average /= static_cast<float>(entry.second.size());
        for (std::size_t idx : entry.second)
            verts[idx].position = average;
    }

    mesh.heal(options.tolerance, options.tolerance);
    mesh.recomputeNormals();
    updateSolidMetadata(solid);
    return true;
}

bool Weld::apply(Curve& curve, const WeldOptions& options) const
{
    auto loop = curve.getBoundaryLoop();
    if (loop.size() < 2)
        return false;
    auto welded = MeshUtils::weldSequential(loop, options.tolerance);
    auto healed = MeshUtils::collapseTinyEdges(welded, options.tolerance);
    return curve.rebuildFromPoints(healed);
}

bool VertexTools::applySoftTranslation(Solid& solid, const std::vector<int>& seedVertices, const SoftSelectionOptions& options) const
{
    if (seedVertices.empty())
        return false;
    auto& mesh = solid.getMesh();
    auto& verts = mesh.getVertices();
    if (verts.empty())
        return false;
    std::vector<Vector3> seeds;
    seeds.reserve(seedVertices.size());
    for (int index : seedVertices) {
        if (index >= 0 && index < static_cast<int>(verts.size())) {
            seeds.push_back(verts[static_cast<std::size_t>(index)].position);
        }
    }
    if (seeds.empty())
        return false;
    bool modified = false;
    float radius = std::max(options.radius, 1e-3f);
    float falloff = std::max(options.falloff, 0.0f);
    Vector3 pivot(0.0f, 0.0f, 0.0f);
    for (const auto& seed : seeds)
        pivot += seed;
    pivot /= static_cast<float>(seeds.size());
    Vector3 axis = options.rotationAxis.lengthSquared() > kEpsilon ? options.rotationAxis.normalized() : Vector3(0.0f, 1.0f, 0.0f);
    float rotationRad = options.rotationDegrees * 3.14159265358979323846f / 180.0f;

    for (auto& vertex : verts) {
        float minDistance = std::numeric_limits<float>::max();
        for (const auto& seed : seeds) {
            float dist = (vertex.position - seed).length();
            minDistance = std::min(minDistance, dist);
        }
        if (minDistance > radius)
            continue;
        float t = 1.0f - (minDistance / radius);
        float weight = falloff <= kEpsilon ? t : std::pow(std::max(0.0f, t), falloff);
        Vector3 translated = vertex.position + options.translation * weight;
        if (std::fabs(rotationRad) > kEpsilon) {
            translated = GeometryTransforms::rotateAroundAxis(translated, pivot, axis, rotationRad * weight);
        }
        Vector3 scaleFactors(
            1.0f + (options.scaling.x - 1.0f) * weight,
            1.0f + (options.scaling.y - 1.0f) * weight,
            1.0f + (options.scaling.z - 1.0f) * weight);
        translated = GeometryTransforms::scaleFromPivot(translated, pivot, scaleFactors);
        vertex.position = translated;
        modified = true;
    }
    if (modified) {
        mesh.recomputeNormals();
        updateSolidMetadata(solid);
    }
    return modified;
}

bool Clean::apply(Solid& solid, const CleanOptions& options) const
{
    auto& mesh = solid.getMesh();
    if (mesh.getVertices().empty())
        return false;
    auto loops = extractFaceLoops(mesh);
    const auto& verts = mesh.getVertices();
    HalfEdgeMesh cleaned;
    std::vector<int> remap(verts.size(), -1);
    auto mapVertex = [&](int index) {
        if (index < 0)
            return -1;
        std::size_t idx = static_cast<std::size_t>(index);
        if (idx >= remap.size())
            return -1;
        if (remap[idx] == -1)
            remap[idx] = cleaned.addVertex(verts[idx].position);
        return remap[idx];
    };

    for (const auto& loop : loops) {
        if (loop.size() < 3)
            continue;
        float area = polygonArea(loop, verts);
        if (area < options.minFaceArea)
            continue;
        std::vector<int> mapped;
        mapped.reserve(loop.size());
        for (int idx : loop) {
            int mappedIndex = mapVertex(idx);
            if (mappedIndex >= 0)
                mapped.push_back(mappedIndex);
        }
        if (mapped.size() >= 3)
            cleaned.addFace(mapped);
    }

    cleaned.heal(options.minEdgeLength, options.minEdgeLength);
    cleaned.recomputeNormals();
    if (cleaned.getVertices().empty())
        return false;
    solid.setMesh(std::move(cleaned));
    updateSolidMetadata(solid);
    return true;
}

bool Clean::apply(Curve& curve, const CleanOptions& options) const
{
    auto loop = curve.getBoundaryLoop();
    if (loop.size() < 3)
        return false;
    auto welded = MeshUtils::weldSequential(loop, options.minEdgeLength);
    auto healed = MeshUtils::collapseTinyEdges(welded, options.minEdgeLength);
    return curve.rebuildFromPoints(healed);
}

void ClothEngine::simulate(Solid& clothSurface, const ClothOptions& options, int frames) const
{
    if (frames <= 0)
        return;
    auto& mesh = clothSurface.getMesh();
    auto& verts = mesh.getVertices();
    if (verts.empty())
        return;
    std::vector<Vector3> velocities(verts.size(), Vector3(0.0f, 0.0f, 0.0f));
    float dt = std::max(options.timestep, 1e-4f);
    auto adjacency = buildAdjacency(mesh);

    std::vector<bool> pinned(verts.size(), false);
    for (int index : options.pinnedVertices) {
        if (index >= 0 && index < static_cast<int>(verts.size()))
            pinned[static_cast<std::size_t>(index)] = true;
    }

    std::vector<float> weights(verts.size(), 1.0f);
    if (!options.weightMap.empty()) {
        for (std::size_t i = 0; i < verts.size() && i < options.weightMap.size(); ++i)
            weights[i] = std::max(0.0f, options.weightMap[i]);
    }

    struct ColliderBounds {
        Vector3 min;
        Vector3 max;
    };
    std::vector<ColliderBounds> colliders;
    for (const Solid* collider : options.colliders) {
        if (!collider)
            continue;
        const auto& cVerts = collider->getMesh().getVertices();
        if (cVerts.empty())
            continue;
        Vector3 min = cVerts.front().position;
        Vector3 max = cVerts.front().position;
        for (const auto& cv : cVerts) {
            min.x = std::min(min.x, cv.position.x);
            min.y = std::min(min.y, cv.position.y);
            min.z = std::min(min.z, cv.position.z);
            max.x = std::max(max.x, cv.position.x);
            max.y = std::max(max.y, cv.position.y);
            max.z = std::max(max.z, cv.position.z);
        }
        colliders.push_back({ min, max });
    }

    std::vector<Vector3> restPositions;
    restPositions.reserve(verts.size());
    for (const auto& v : verts)
        restPositions.push_back(v.position);

    for (int frame = 0; frame < frames; ++frame) {
        for (std::size_t i = 0; i < verts.size(); ++i) {
            if (pinned[i]) {
                verts[i].position = restPositions[i];
                velocities[i] = Vector3(0.0f, 0.0f, 0.0f);
                continue;
            }
            float weight = i < weights.size() ? weights[i] : 1.0f;
            velocities[i] += options.gravity * dt * weight;
            velocities[i] *= (1.0f - options.damping);
            verts[i].position += velocities[i] * dt;
        }

        for (std::size_t i = 0; i < verts.size(); ++i) {
            if (pinned[i])
                continue;
            for (const auto& bounds : colliders) {
                Vector3 pos = verts[i].position;
                if (pos.x >= bounds.min.x && pos.x <= bounds.max.x && pos.y >= bounds.min.y && pos.y <= bounds.max.y
                    && pos.z >= bounds.min.z && pos.z <= bounds.max.z) {
                    verts[i].position.y = bounds.max.y + 1e-3f;
                    velocities[i].y = std::max(0.0f, velocities[i].y);
                }
            }
        }

        for (int iter = 0; iter < options.solverIterations; ++iter) {
            for (std::size_t i = 0; i < verts.size(); ++i) {
                if (pinned[i])
                    continue;
                const auto& neighbors = adjacency[i];
                if (neighbors.empty())
                    continue;
                Vector3 centroid(0.0f, 0.0f, 0.0f);
                for (int n : neighbors)
                    centroid += verts[static_cast<std::size_t>(n)].position;
                centroid /= static_cast<float>(neighbors.size());
                verts[i].position = verts[i].position * options.stiffness + centroid * (1.0f - options.stiffness);
            }
        }
    }
    mesh.recomputeNormals();
    updateSolidMetadata(clothSurface);
}

CADDesigner::CADDesigner(GeometryKernel& kernel)
    : geometry(kernel)
{
}

Solid* CADDesigner::revolve(const Curve& profile, const RevolveOptions& options) const
{
    auto loop = profile.getBoundaryLoop();
    if (loop.size() < 3)
        return nullptr;
    int segments = std::max(3, options.segments);
    float angleRad = options.angleDegrees * 3.14159265358979323846f / 180.0f;
    Vector3 axisDir = options.axisDirection.normalized();
    if (axisDir.lengthSquared() <= kEpsilon)
        return nullptr;
    HalfEdgeMesh mesh;
    std::vector<std::vector<int>> rings;
    rings.reserve(static_cast<std::size_t>(segments) + 1);
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float angle = angleRad * t;
        std::vector<int> ring;
        ring.reserve(loop.size());
        for (const auto& p : loop) {
            Vector3 rotated = GeometryTransforms::rotateAroundAxis(p, options.axisPoint, axisDir, angle);
            ring.push_back(mesh.addVertex(rotated));
        }
        rings.push_back(std::move(ring));
    }
    bool ok = true;
    for (std::size_t i = 0; i + 1 < rings.size(); ++i) {
        if (!connectRings(mesh, rings[i], rings[i + 1])) {
            ok = false;
        }
    }
    if (options.angleDegrees < 360.0f - 1e-3f) {
        ok &= addLoopFace(mesh, rings.front(), true);
        ok &= addLoopFace(mesh, rings.back(), false);
    } else {
        connectRings(mesh, rings.back(), rings.front());
    }
    mesh.heal(kEpsilon, kEpsilon);
    if (mesh.getVertices().empty() || !ok)
        return nullptr;
    auto solidPtr = Solid::createFromMesh(std::move(mesh));
    if (!solidPtr)
        return nullptr;
    Solid* raw = static_cast<Solid*>(geometry.addObject(std::move(solidPtr)));
    if (!raw)
        return nullptr;
    updateSolidMetadata(*raw);
    return raw;
}

Solid* CADDesigner::sweep(const Curve& profile, const std::vector<Vector3>& path, const SweepOptions& options) const
{
    if (path.size() < 2)
        return nullptr;
    auto loop = profile.getBoundaryLoop();
    if (loop.size() < 3)
        return nullptr;
    float spacing = std::max(0.05f, 1.0f / std::max(1, options.samples));
    std::vector<Vector3> sampledPath = densifyPath(path, spacing);
    if (sampledPath.size() < 2)
        sampledPath = path;
    std::vector<std::vector<Vector3>> sections;
    sections.reserve(sampledPath.size());
    Vector3 baseCentroid(0.0f, 0.0f, 0.0f);
    for (const auto& p : loop)
        baseCentroid += p;
    baseCentroid /= static_cast<float>(loop.size());
    for (const auto& point : sampledPath) {
        std::vector<Vector3> ring;
        ring.reserve(loop.size());
        for (const auto& vertex : loop) {
            Vector3 offset = vertex - baseCentroid;
            ring.push_back(point + offset);
        }
        sections.push_back(std::move(ring));
    }
    HalfEdgeMesh mesh = buildSkinMesh(sections, false, options.capEnds, options.capEnds);
    if (mesh.getVertices().empty())
        return nullptr;
    auto solidPtr = Solid::createFromMesh(std::move(mesh));
    if (!solidPtr)
        return nullptr;
    Solid* raw = static_cast<Solid*>(geometry.addObject(std::move(solidPtr)));
    if (!raw)
        return nullptr;
    updateSolidMetadata(*raw);
    return raw;
}

Solid* CADDesigner::mirror(const Solid& solid, const Vector3& planePoint, const Vector3& planeNormal) const
{
    Vector3 normal = planeNormal.normalized();
    if (normal.lengthSquared() <= kEpsilon)
        return nullptr;
    HalfEdgeMesh mesh = solid.getMesh();
    for (auto& vertex : mesh.getVertices()) {
        Vector3 relative = vertex.position - planePoint;
        float distance = relative.dot(normal);
        vertex.position = vertex.position - normal * (2.0f * distance);
    }
    mesh.heal(kEpsilon, kEpsilon);
    if (mesh.getVertices().empty())
        return nullptr;
    auto mirrored = Solid::createFromMesh(std::move(mesh));
    if (!mirrored)
        return nullptr;
    Solid* raw = static_cast<Solid*>(geometry.addObject(std::move(mirrored)));
    if (!raw)
        return nullptr;
    updateSolidMetadata(*raw);
    return raw;
}

Solid* CADDesigner::shell(const Solid& solid, const ShellOptions& options) const
{
    GeometryObject* copy = geometry.cloneObject(solid);
    if (!copy || copy->getType() != GeometryObject::ObjectType::Solid)
        return nullptr;
    Solid* shellSolid = static_cast<Solid*>(copy);
    PushAndPull thickener;
    PushPullOptions opts;
    opts.distance = options.thickness;
    opts.createCaps = options.capHoles;
    opts.generateInnerShell = true;
    if (!thickener.thicken(*shellSolid, opts)) {
        geometry.deleteObject(shellSolid);
        return nullptr;
    }
    return shellSolid;
}

std::vector<Solid*> CADDesigner::pattern(const Solid& solid, const PatternOptions& options) const
{
    std::vector<Solid*> instances;
    if (options.count <= 0)
        return instances;
    for (int i = 0; i < options.count; ++i) {
        GeometryObject* copy = geometry.cloneObject(solid);
        if (!copy || copy->getType() != GeometryObject::ObjectType::Solid)
            continue;
        Solid* inst = static_cast<Solid*>(copy);
        Vector3 offset = options.translationStep * static_cast<float>(i);
        inst->translate(offset);
        if (std::fabs(options.rotationStepDegrees) > kEpsilon) {
            float angle = options.rotationStepDegrees * 3.14159265358979323846f / 180.0f * static_cast<float>(i);
            inst->rotate(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), angle);
        }
        instances.push_back(inst);
    }
    return instances;
}

Solid* CADDesigner::split(const Solid& solid, const SplitOptions& options) const
{
    GeometryObject* copy = geometry.cloneObject(solid);
    if (!copy || copy->getType() != GeometryObject::ObjectType::Solid)
        return nullptr;
    Solid* splitSolid = static_cast<Solid*>(copy);
    auto& mesh = splitSolid->getMesh();
    auto loops = extractFaceLoops(mesh);
    const auto& verts = mesh.getVertices();
    HalfEdgeMesh filtered;
    std::vector<int> remap(verts.size(), -1);
    Vector3 normal = options.planeNormal.lengthSquared() > kEpsilon ? options.planeNormal.normalized() : Vector3(0.0f, 1.0f, 0.0f);

    auto mapVertex = [&](int idx) {
        if (idx < 0)
            return -1;
        std::size_t index = static_cast<std::size_t>(idx);
        if (index >= remap.size())
            return -1;
        if (remap[index] == -1)
            remap[index] = filtered.addVertex(verts[index].position);
        return remap[index];
    };

    for (const auto& loop : loops) {
        Vector3 centroid(0.0f, 0.0f, 0.0f);
        for (int idx : loop)
            centroid += verts[static_cast<std::size_t>(idx)].position;
        centroid /= static_cast<float>(loop.size());
        float side = (centroid - options.planePoint).dot(normal);
        if ((side >= 0.0f) != options.keepPositive)
            continue;
        std::vector<int> mapped;
        mapped.reserve(loop.size());
        for (int idx : loop) {
            int mappedIdx = mapVertex(idx);
            if (mappedIdx >= 0)
                mapped.push_back(mappedIdx);
        }
        if (mapped.size() >= 3)
            filtered.addFace(mapped);
    }

    filtered.heal(kEpsilon, kEpsilon);
    filtered.recomputeNormals();
    if (filtered.getVertices().empty()) {
        geometry.deleteObject(splitSolid);
        return nullptr;
    }
    splitSolid->setMesh(std::move(filtered));
    updateSolidMetadata(*splitSolid);
    return splitSolid;
}

Curve* CADDesigner::imprint(const Solid& solid, const std::vector<Vector3>& path) const
{
    Surface surface(geometry);
    SurfaceDrawOptions opts;
    opts.remesh = false;
    opts.samplingDistance = 0.05f;
    return surface.drawPolylineOnSolid(solid, path, opts);
}

} // namespace Phase6
