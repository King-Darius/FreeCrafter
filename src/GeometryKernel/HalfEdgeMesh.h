#pragma once
#include <vector>
#include <unordered_map>
#include "Vector3.h"

struct HalfEdgeVertex {
    Vector3 position;
    int halfEdge = -1;
};

struct HalfEdgeRecord {
    int origin = -1;
    int destination = -1;
    int face = -1;
    int next = -1;
    int opposite = -1;
};

struct HalfEdgeFace {
    int halfEdge = -1;
    Vector3 normal;
};

struct HalfEdgeTriangle {
    int v0 = -1;
    int v1 = -1;
    int v2 = -1;
    Vector3 normal;
};

class HalfEdgeMesh {
public:
    int addVertex(const Vector3& position);
    int addFace(const std::vector<int>& loop);
    void clear();

    bool isManifold() const;

    const std::vector<HalfEdgeVertex>& getVertices() const { return vertices; }
    std::vector<HalfEdgeVertex>& getVertices() { return vertices; }

    const std::vector<HalfEdgeRecord>& getHalfEdges() const { return halfEdges; }
    const std::vector<HalfEdgeFace>& getFaces() const { return faces; }
    const std::vector<HalfEdgeTriangle>& getTriangles() const { return triangles; }

private:
    Vector3 computeFaceNormal(const std::vector<int>& loop) const;

    std::vector<HalfEdgeVertex> vertices;
    std::vector<HalfEdgeRecord> halfEdges;
    std::vector<HalfEdgeFace> faces;
    std::vector<HalfEdgeTriangle> triangles;
    std::unordered_map<long long, int> directedEdgeMap;
};
