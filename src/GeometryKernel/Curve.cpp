#include "Curve.h"
#include "HalfEdgeMesh.h"
#include "MeshUtils.h"
#include <algorithm>

namespace {
constexpr float kDefaultTolerance = 1e-4f;
}

Curve::Curve(std::vector<Vector3> loop, HalfEdgeMesh mesh)
    : boundaryLoop(std::move(loop)), mesh(std::move(mesh)) {}

std::unique_ptr<Curve> Curve::createFromPoints(const std::vector<Vector3>& pts) {
    auto welded = MeshUtils::weldSequential(pts, kDefaultTolerance);
    auto healed = MeshUtils::collapseTinyEdges(welded, kDefaultTolerance);
    if (healed.size() < 3) {
        return nullptr;
    }

    Vector3 normal = MeshUtils::computePolygonNormal(healed);
    if (normal.length() <= 1e-6f) {
        return nullptr;
    }
    if (normal.y < 0.0f) {
        std::reverse(healed.begin(), healed.end());
    }

    HalfEdgeMesh mesh;
    std::vector<int> loopIndices;
    loopIndices.reserve(healed.size());
    for (const auto& p : healed) {
        loopIndices.push_back(mesh.addVertex(p));
    }

    if (mesh.addFace(loopIndices) < 0) {
        return nullptr;
    }

    if (!mesh.isManifold()) {
        return nullptr;
    }

    return std::unique_ptr<Curve>(new Curve(std::move(healed), std::move(mesh)));
}
