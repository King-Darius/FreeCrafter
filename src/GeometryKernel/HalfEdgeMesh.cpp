#include "HalfEdgeMesh.h"
#include <algorithm>
#include <utility>

namespace {
inline long long makeEdgeKey(int from, int to) {
    return (static_cast<long long>(from) << 32) | static_cast<unsigned int>(to);
}

inline long long makeUndirectedKey(int a, int b) {
    if (a > b) std::swap(a, b);
    return (static_cast<long long>(a) << 32) | static_cast<unsigned int>(b);
}
}

int HalfEdgeMesh::addVertex(const Vector3& position) {
    int index = static_cast<int>(vertices.size());
    vertices.push_back({ position, -1 });
    return index;
}

int HalfEdgeMesh::addFace(const std::vector<int>& loop) {
    if (loop.size() < 3) return -1;
    int faceIndex = static_cast<int>(faces.size());
    HalfEdgeFace face;
    face.halfEdge = static_cast<int>(halfEdges.size());
    faces.push_back(face);

    size_t start = halfEdges.size();
    size_t count = loop.size();
    std::vector<std::pair<int, int>> touchedVertices;
    std::vector<std::pair<int, int>> updatedOpposites;
    std::vector<long long> insertedKeys;
    touchedVertices.reserve(count);
    updatedOpposites.reserve(count);
    insertedKeys.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        int origin = loop[i];
        int destination = loop[(i + 1) % count];
        HalfEdgeRecord record;
        record.origin = origin;
        record.destination = destination;
        record.face = faceIndex;
        record.next = static_cast<int>(start + (i + 1) % count);
        halfEdges.push_back(record);
        int previous = vertices[origin].halfEdge;
        if (previous == -1) {
            touchedVertices.emplace_back(origin, previous);
            vertices[origin].halfEdge = static_cast<int>(halfEdges.size() - 1);
        }
        long long key = makeEdgeKey(origin, destination);
        auto it = directedEdgeMap.find(key);
        if (it != directedEdgeMap.end()) {
            // rollback and reject face
            for (const auto& tv : touchedVertices) {
                vertices[tv.first].halfEdge = tv.second;
            }
            for (const auto& oppEntry : updatedOpposites) {
                halfEdges[oppEntry.first].opposite = oppEntry.second;
            }
            halfEdges.resize(start);
            faces.pop_back();
            for (long long inserted : insertedKeys) {
                directedEdgeMap.erase(inserted);
            }
            return -1;
        }
        directedEdgeMap[key] = static_cast<int>(halfEdges.size() - 1);
        insertedKeys.push_back(key);
        long long oppositeKey = makeEdgeKey(destination, origin);
        auto opp = directedEdgeMap.find(oppositeKey);
        if (opp != directedEdgeMap.end()) {
            int previousOpp = halfEdges[opp->second].opposite;
            updatedOpposites.emplace_back(opp->second, previousOpp);
            halfEdges[opp->second].opposite = static_cast<int>(halfEdges.size() - 1);
            halfEdges.back().opposite = opp->second;
        }
    }

    faces.back().normal = computeFaceNormal(loop);

    // triangulate face using simple fan
    if (loop.size() >= 3) {
        for (size_t i = 1; i + 1 < loop.size(); ++i) {
            HalfEdgeTriangle tri;
            tri.v0 = loop[0];
            tri.v1 = loop[i];
            tri.v2 = loop[i + 1];
            tri.normal = faces.back().normal;
            triangles.push_back(tri);
        }
    }

    return faceIndex;
}

void HalfEdgeMesh::clear() {
    vertices.clear();
    halfEdges.clear();
    faces.clear();
    triangles.clear();
    directedEdgeMap.clear();
}

bool HalfEdgeMesh::isManifold() const {
    std::unordered_map<long long, int> undirectedCounts;
    for (size_t i = 0; i < halfEdges.size(); ++i) {
        const auto& edge = halfEdges[i];
        if (edge.origin < 0 || edge.destination < 0) continue;
        long long key = makeUndirectedKey(edge.origin, edge.destination);
        int count = ++undirectedCounts[key];
        if (count > 2) return false;
        if (count == 2) {
            if (edge.opposite == -1) return false;
            const auto& opp = halfEdges[edge.opposite];
            if (opp.opposite != static_cast<int>(i)) return false;
        }
    }
    return true;
}

Vector3 HalfEdgeMesh::computeFaceNormal(const std::vector<int>& loop) const {
    Vector3 normal(0.0f, 0.0f, 0.0f);
    if (loop.size() < 3) return normal;
    for (size_t i = 0; i < loop.size(); ++i) {
        const Vector3& current = vertices[loop[i]].position;
        const Vector3& next = vertices[loop[(i + 1) % loop.size()]].position;
        normal.x += (current.y - next.y) * (current.z + next.z);
        normal.y += (current.z - next.z) * (current.x + next.x);
        normal.z += (current.x - next.x) * (current.y + next.y);
    }
    return normal.normalized();
}

void HalfEdgeMesh::recomputeNormals()
{
    for (auto& face : faces) {
        if (face.halfEdge < 0) {
            face.normal = Vector3();
            continue;
        }
        std::vector<int> loop;
        int start = face.halfEdge;
        int current = start;
        loop.clear();
        do {
            if (current < 0 || current >= static_cast<int>(halfEdges.size())) {
                loop.clear();
                break;
            }
            const HalfEdgeRecord& edge = halfEdges[current];
            loop.push_back(edge.origin);
            current = edge.next;
        } while (current != start && current != -1);
        if (loop.size() >= 3) {
            face.normal = computeFaceNormal(loop);
        } else {
            face.normal = Vector3();
        }
    }

    for (auto& tri : triangles) {
        if (tri.v0 < 0 || tri.v1 < 0 || tri.v2 < 0) {
            tri.normal = Vector3();
            continue;
        }
        const Vector3& a = vertices[tri.v0].position;
        const Vector3& b = vertices[tri.v1].position;
        const Vector3& c = vertices[tri.v2].position;
        Vector3 normal = (b - a).cross(c - a);
        if (normal.lengthSquared() > 1e-8f) {
            tri.normal = normal.normalized();
        } else {
            tri.normal = Vector3();
        }
    }
}
