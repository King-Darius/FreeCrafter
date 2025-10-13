#include "Solid.h"
#include "Curve.h"
#include "HalfEdgeMesh.h"
#include "MeshUtils.h"
#include "TransformUtils.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float kDefaultTolerance = 1e-4f;

std::vector<Vector3> prepareProfile(const std::vector<Vector3>& baseProfile) {
    auto welded = MeshUtils::weldSequential(baseProfile, kDefaultTolerance);
    auto healed = MeshUtils::collapseTinyEdges(welded, kDefaultTolerance);
    return healed;
}
}

Solid::Solid(std::vector<Vector3> base, float h, HalfEdgeMesh meshData)
    : baseLoop(std::move(base)), height(h), mesh(std::move(meshData)) {}

std::unique_ptr<Solid> Solid::createFromProfile(const std::vector<Vector3>& baseProfile, float height,
    bool capStart, bool capEnd)
{
    if (std::fabs(height) <= kDefaultTolerance) {
        return nullptr;
    }
    return createFromProfileWithVector(baseProfile, Vector3(0.0f, height, 0.0f), capStart, capEnd);
}

std::unique_ptr<Solid> Solid::createFromProfileWithVector(const std::vector<Vector3>& baseProfile,
    const Vector3& direction, bool capStart, bool capEnd)
{
    auto profile = prepareProfile(baseProfile);
    if (profile.size() < 3) {
        return nullptr;
    }

    float directionLength = direction.length();
    if (directionLength <= kDefaultTolerance) {
        return nullptr;
    }

    Vector3 normal = MeshUtils::computePolygonNormal(profile);
    if (normal.length() <= 1e-6f) {
        return nullptr;
    }

    Vector3 directionUnit = direction / directionLength;
    if (normal.dot(directionUnit) < 0.0f) {
        std::reverse(profile.begin(), profile.end());
        normal = -normal;
    }

    HalfEdgeMesh mesh;
    std::vector<int> bottomIndices;
    std::vector<int> topIndices;
    bottomIndices.reserve(profile.size());
    topIndices.reserve(profile.size());

    for (const auto& p : profile) {
        bottomIndices.push_back(mesh.addVertex(p));
    }
    for (const auto& p : profile) {
        Vector3 topPoint = p + direction;
        topIndices.push_back(mesh.addVertex(topPoint));
    }

    if (capStart) {
        std::vector<int> bottomLoop = bottomIndices;
        std::reverse(bottomLoop.begin(), bottomLoop.end());
        if (mesh.addFace(bottomLoop) < 0) {
            return nullptr;
        }
    }

    if (capEnd) {
        if (mesh.addFace(topIndices) < 0) {
            return nullptr;
        }
    }

    for (size_t i = 0; i < profile.size(); ++i) {
        size_t next = (i + 1) % profile.size();
        std::vector<int> quad = {
            bottomIndices[i],
            bottomIndices[next],
            topIndices[next],
            topIndices[i]
        };
        if (mesh.addFace(quad) < 0) {
            return nullptr;
        }
    }

    mesh.heal(kDefaultTolerance, kDefaultTolerance);

    if (!mesh.isManifold()) {
        return nullptr;
    }

    return std::unique_ptr<Solid>(new Solid(std::move(profile), directionLength, std::move(mesh)));
}

std::unique_ptr<Solid> Solid::createFromCurve(const Curve& curve, float height, bool capStart, bool capEnd)
{
    return createFromProfile(curve.getBoundaryLoop(), height, capStart, capEnd);
}

std::unique_ptr<Solid> Solid::createFromCurveWithVector(const Curve& curve, const Vector3& direction,
    bool capStart, bool capEnd)
{
    return createFromProfileWithVector(curve.getBoundaryLoop(), direction, capStart, capEnd);
}

std::unique_ptr<Solid> Solid::createFromMesh(HalfEdgeMesh meshData)
{
    meshData.heal(kDefaultTolerance, kDefaultTolerance);
    if (!meshData.isManifold()) {
        return nullptr;
    }

    const auto& verts = meshData.getVertices();
    if (verts.empty()) {
        return nullptr;
    }

    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (const auto& v : verts) {
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
    }

    std::vector<Vector3> base;
    for (const auto& v : verts) {
        if (std::abs(v.position.y - minY) <= kDefaultTolerance * 10.0f) {
            base.push_back(Vector3(v.position.x, minY, v.position.z));
        }
    }
    if (base.size() < 3) {
        // fall back to projecting entire mesh
        base.clear();
        for (const auto& v : verts) {
            base.push_back(Vector3(v.position.x, minY, v.position.z));
        }
    }

    float height = std::max(kDefaultTolerance, maxY - minY);
    return std::unique_ptr<Solid>(new Solid(std::move(base), height, std::move(meshData)));
}

void Solid::applyTransform(const std::function<Vector3(const Vector3&)>& fn)
{
    for (auto& point : baseLoop) {
        point = fn(point);
    }
    mesh.transformVertices(fn);
}

void Solid::translate(const Vector3& delta)
{
    applyTransform([&](const Vector3& p) { return GeometryTransforms::translate(p, delta); });
}

void Solid::rotate(const Vector3& pivot, const Vector3& axis, float angleRadians)
{
    applyTransform([&](const Vector3& p) { return GeometryTransforms::rotateAroundAxis(p, pivot, axis, angleRadians); });
}

void Solid::scale(const Vector3& pivot, const Vector3& factors)
{
    applyTransform([&](const Vector3& p) { return GeometryTransforms::scaleFromPivot(p, pivot, factors); });
}

std::unique_ptr<GeometryObject> Solid::clone() const
{
    auto copy = std::unique_ptr<Solid>(new Solid(baseLoop, height, mesh));
    copy->setSelected(isSelected());
    copy->setVisible(isVisible());
    copy->setHidden(isHidden());
    copy->setStableId(0);
    return copy;
}

void Solid::setMesh(HalfEdgeMesh meshData)
{
    mesh = std::move(meshData);
    mesh.heal(kDefaultTolerance, kDefaultTolerance);
}

void Solid::setBaseMetadata(std::vector<Vector3> base, float newHeight)
{
    baseLoop = std::move(base);
    height = std::max(newHeight, kDefaultTolerance);
}
