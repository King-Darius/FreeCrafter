#include "Solid.h"
#include "Curve.h"
#include "HalfEdgeMesh.h"
#include "MeshUtils.h"
#include "TransformUtils.h"
#include <algorithm>

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

std::unique_ptr<Solid> Solid::createFromProfile(const std::vector<Vector3>& baseProfile, float height) {
    if (height <= kDefaultTolerance) {
        return nullptr;
    }

    auto profile = prepareProfile(baseProfile);
    if (profile.size() < 3) {
        return nullptr;
    }

    Vector3 normal = MeshUtils::computePolygonNormal(profile);
    if (normal.length() <= 1e-6f) {
        return nullptr;
    }
    if (normal.y < 0.0f) {
        std::reverse(profile.begin(), profile.end());
    }

    HalfEdgeMesh mesh;
    std::vector<int> bottomIndices;
    std::vector<int> topIndices;
    bottomIndices.reserve(profile.size());
    topIndices.reserve(profile.size());
    for (const auto& p : profile) {
        bottomIndices.push_back(mesh.addVertex(Vector3(p.x, 0.0f, p.z)));
    }
    for (const auto& p : profile) {
        topIndices.push_back(mesh.addVertex(Vector3(p.x, height, p.z)));
    }

    std::vector<int> bottomLoop = bottomIndices;
    std::reverse(bottomLoop.begin(), bottomLoop.end());
    if (mesh.addFace(bottomLoop) < 0) {
        return nullptr;
    }

    if (mesh.addFace(topIndices) < 0) {
        return nullptr;
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

    if (!mesh.isManifold()) {
        return nullptr;
    }

    return std::unique_ptr<Solid>(new Solid(std::move(profile), height, std::move(mesh)));
}

std::unique_ptr<Solid> Solid::createFromCurve(const Curve& curve, float height) {
    return createFromProfile(curve.getBoundaryLoop(), height);
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
