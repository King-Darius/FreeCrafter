#include "Curve.h"
#include "HalfEdgeMesh.h"
#include "MeshUtils.h"
#include "TransformUtils.h"
#include <algorithm>

namespace {
constexpr float kDefaultTolerance = 1e-4f;

std::vector<Vector3> sanitizePoints(const std::vector<Vector3>& pts)
{
    auto welded = MeshUtils::weldSequential(pts, kDefaultTolerance);
    return MeshUtils::collapseTinyEdges(welded, kDefaultTolerance);
}

bool hasMeaningfulLength(const std::vector<Vector3>& pts)
{
    if (pts.size() < 2)
        return false;
    const float minSegmentSq = kDefaultTolerance * kDefaultTolerance;
    for (size_t i = 1; i < pts.size(); ++i) {
        if (MeshUtils::distanceSquared(pts[i], pts[i - 1]) > minSegmentSq)
            return true;
    }
    return false;
}

bool populatePolylineData(std::vector<Vector3>&& sanitized,
                          const std::vector<bool>& hardnessOverride,
                          std::vector<Vector3>& boundaryOut,
                          HalfEdgeMesh& meshOut,
                          std::vector<bool>& hardnessOut)
{
    if (sanitized.size() < 2)
        return false;
    if (!hasMeaningfulLength(sanitized))
        return false;

    meshOut.clear();
    for (const auto& point : sanitized)
        meshOut.addVertex(point);

    boundaryOut = std::move(sanitized);
    if (!hardnessOverride.empty()) {
        hardnessOut = hardnessOverride;
        hardnessOut.resize(boundaryOut.size(), false);
    } else {
        hardnessOut.assign(boundaryOut.size(), false);
    }
    return true;
}

std::unique_ptr<Curve> makePolyline(std::vector<Vector3>&& sanitized,
                                    const std::vector<bool>& hardnessOverride)
{
    std::vector<Vector3> boundary;
    HalfEdgeMesh mesh;
    std::vector<bool> hardness;
    if (!populatePolylineData(std::move(sanitized), hardnessOverride, boundary, mesh, hardness))
        return nullptr;
    return std::unique_ptr<Curve>(new Curve(std::move(boundary), std::move(mesh), std::move(hardness)));
}
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
    auto healed = sanitizePoints(pts);
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
    auto sanitized = sanitizePoints(pts);
    if (sanitized.size() < 2)
        return nullptr;

    if (sanitized.size() < 3) {
        return makePolyline(std::move(sanitized), edgeHardness);
    }

    std::vector<Vector3> loop;
    HalfEdgeMesh mesh;
    std::vector<bool> hardness;
    if (!buildCurve(sanitized, loop, mesh, hardness)) {
        return makePolyline(std::move(sanitized), edgeHardness);
    }
    if (!edgeHardness.empty()) {
        hardness = edgeHardness;
        hardness.resize(loop.size(), false);
    }
    return std::unique_ptr<Curve>(new Curve(std::move(loop), std::move(mesh), std::move(hardness)));
}

bool Curve::rebuildFromPoints(const std::vector<Vector3>& pts, const std::vector<bool>& edgeHardness)
{
    auto sanitized = sanitizePoints(pts);
    if (sanitized.size() < 2)
        return false;

    if (sanitized.size() < 3) {
        if (!populatePolylineData(std::move(sanitized), edgeHardness, boundaryLoop, mesh, hardnessFlags))
            return false;
        return true;
    }

    std::vector<Vector3> loop;
    HalfEdgeMesh rebuiltMesh;
    std::vector<bool> hardness;
    if (!buildCurve(sanitized, loop, rebuiltMesh, hardness)) {
        if (!populatePolylineData(std::move(sanitized), edgeHardness, boundaryLoop, mesh, hardnessFlags))
            return false;
        return true;
    }
    boundaryLoop = std::move(loop);
    rebuiltMesh.heal(kDefaultTolerance, kDefaultTolerance);
    mesh = std::move(rebuiltMesh);
    if (!edgeHardness.empty()) {
        hardness = edgeHardness;
        hardness.resize(boundaryLoop.size(), false);
    } else {
        hardness.resize(boundaryLoop.size(), false);
    }
    hardnessFlags = std::move(hardness);
    return true;
}

std::unique_ptr<Curve> Curve::createOpenPolyline(const std::vector<Vector3>& pts,
    const std::vector<bool>& edgeHardness)
{
    auto sanitized = sanitizePoints(pts);
    if (sanitized.size() < 2)
        return nullptr;
    return makePolyline(std::move(sanitized), edgeHardness);
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
