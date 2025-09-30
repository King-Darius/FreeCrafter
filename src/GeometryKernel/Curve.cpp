#include "Curve.h"
#include "HalfEdgeMesh.h"
#include "MeshUtils.h"
#include "TransformUtils.h"
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

    mesh.heal(kDefaultTolerance, kDefaultTolerance);

    if (!mesh.isManifold()) {
        return nullptr;
    }

    return std::unique_ptr<Curve>(new Curve(std::move(healed), std::move(mesh)));
}

void Curve::applyTransform(const std::function<Vector3(const Vector3&)>& fn)
{
    for (auto& point : boundaryLoop) {
        point = fn(point);
    }
    mesh.transformVertices(fn);
}

void Curve::translate(const Vector3& delta)
{
    applyTransform([&](const Vector3& p) { return GeometryTransforms::translate(p, delta); });
}

void Curve::rotate(const Vector3& pivot, const Vector3& axis, float angleRadians)
{
    applyTransform([&](const Vector3& p) { return GeometryTransforms::rotateAroundAxis(p, pivot, axis, angleRadians); });
}

void Curve::scale(const Vector3& pivot, const Vector3& factors)
{
    applyTransform([&](const Vector3& p) { return GeometryTransforms::scaleFromPivot(p, pivot, factors); });
}
