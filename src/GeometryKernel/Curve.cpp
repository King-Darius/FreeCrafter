#include "Curve.h"
#include "HalfEdgeMesh.h"
#include "MeshUtils.h"
#include "TransformUtils.h"
#include <algorithm>

namespace {
constexpr float kDefaultTolerance = 1e-4f;
}

Curve::Curve(std::vector<Vector3> loop, HalfEdgeMesh mesh, std::vector<bool> hardness)
    : boundaryLoop(std::move(loop))
    , mesh(std::move(mesh))
    , hardnessFlags(std::move(hardness))
{
    if (hardnessFlags.empty()) {
        hardnessFlags.assign(boundaryLoop.size(), false);
    }
    if (hardnessFlags.size() != boundaryLoop.size()) {
        hardnessFlags.resize(boundaryLoop.size(), false);
    }
}

namespace {
bool buildCurve(const std::vector<Vector3>& pts, std::vector<Vector3>& outLoop, HalfEdgeMesh& outMesh,
    std::vector<bool>& hardness)
{
    auto welded = MeshUtils::weldSequential(pts, kDefaultTolerance);
    auto healed = MeshUtils::collapseTinyEdges(welded, kDefaultTolerance);
    if (healed.size() < 3) {
        return false;
    }

    Vector3 normal = MeshUtils::computePolygonNormal(healed);
    if (normal.length() <= 1e-6f) {
        return false;
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
        return false;
    }

    mesh.heal(kDefaultTolerance, kDefaultTolerance);

    if (!mesh.isManifold()) {
        return false;
    }

    outLoop = std::move(healed);
    outMesh = std::move(mesh);
    hardness.assign(outLoop.size(), false);
    return true;
}
}

std::unique_ptr<Curve> Curve::createFromPoints(const std::vector<Vector3>& pts, const std::vector<bool>& edgeHardness) {
    std::vector<Vector3> loop;
    HalfEdgeMesh mesh;
    std::vector<bool> hardness;
    if (!buildCurve(pts, loop, mesh, hardness)) {
        return nullptr;
    }
    if (!edgeHardness.empty()) {
        hardness = edgeHardness;
        hardness.resize(loop.size(), false);
    }
    return std::unique_ptr<Curve>(new Curve(std::move(loop), std::move(mesh), std::move(hardness)));
}

bool Curve::rebuildFromPoints(const std::vector<Vector3>& pts, const std::vector<bool>& edgeHardness)
{
    std::vector<Vector3> loop;
    HalfEdgeMesh mesh;
    std::vector<bool> hardness;
    if (!buildCurve(pts, loop, mesh, hardness)) {
        return false;
    }
    boundaryLoop = std::move(loop);
    mesh.heal(kDefaultTolerance, kDefaultTolerance);
    this->mesh = std::move(mesh);
    if (!edgeHardness.empty()) {
        hardness = edgeHardness;
        hardness.resize(boundaryLoop.size(), false);
    } else {
        hardness.resize(boundaryLoop.size(), false);
    }
    hardnessFlags = std::move(hardness);
    return true;
}

std::unique_ptr<GeometryObject> Curve::clone() const
{
    auto copy = std::unique_ptr<Curve>(new Curve(boundaryLoop, mesh, hardnessFlags));
    copy->setSelected(isSelected());
    copy->setVisible(isVisible());
    copy->setHidden(isHidden());
    return copy;
}

void Curve::applyTransform(const std::function<Vector3(const Vector3&)>& fn)
{
    for (auto& point : boundaryLoop) {
        point = fn(point);
    }
    mesh.transformVertices(fn);
}

void Curve::setEdgeHardness(std::vector<bool> hardness)
{
    if (hardness.empty()) {
        hardnessFlags.assign(boundaryLoop.size(), false);
        return;
    }
    hardnessFlags = std::move(hardness);
    hardnessFlags.resize(boundaryLoop.size(), false);
}

void Curve::tagAllEdgesHard(bool hard)
{
    hardnessFlags.assign(boundaryLoop.size(), hard);
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
