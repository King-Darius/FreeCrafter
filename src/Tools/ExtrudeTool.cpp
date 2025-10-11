#include "ExtrudeTool.h"

#include "ToolCommands.h"
#include "ToolGeometryUtils.h"
#include "../CameraNavigation.h"
#include "../Scene/Document.h"
#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/MeshUtils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace {
constexpr float kMinimumDistance = 1e-5f;

bool loopIsClosed(const std::vector<Vector3>& loop)
{
    if (loop.size() < 3)
        return false;
    Vector3 start = loop.front();
    Vector3 end = loop.back();
    return (start - end).lengthSquared() <= (kMinimumDistance * kMinimumDistance);
}

bool intersectRayTriangle(const Vector3& origin, const Vector3& direction,
    const Vector3& v0, const Vector3& v1, const Vector3& v2, float& outT)
{
    const float epsilon = 1e-6f;
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 pvec = direction.cross(edge2);
    float det = edge1.dot(pvec);
    if (std::fabs(det) < epsilon)
        return false;
    float invDet = 1.0f / det;
    Vector3 tvec = origin - v0;
    float u = tvec.dot(pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;
    Vector3 qvec = tvec.cross(edge1);
    float v = direction.dot(qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return false;
    float t = edge2.dot(qvec) * invDet;
    if (t < 0.0f)
        return false;
    outT = t;
    return true;
}

float closestDistanceSquaredRaySegment(const Vector3& rayOrigin, const Vector3& rayDirection,
    const Vector3& a, const Vector3& b, float& outRayT)
{
    Vector3 u = b - a;
    Vector3 v = rayDirection;
    Vector3 w0 = a - rayOrigin;

    float aDot = u.dot(u);
    float bDot = u.dot(v);
    float cDot = v.dot(v);
    float dDot = u.dot(w0);
    float eDot = v.dot(w0);
    float denom = aDot * cDot - bDot * bDot;

    float sN, sD = denom;
    float tN, tD = denom;
    const float epsilon = 1e-8f;

    if (denom < epsilon) {
        sN = 0.0f;
        sD = 1.0f;
        tN = eDot;
        tD = cDot;
    } else {
        sN = (bDot * eDot - cDot * dDot);
        tN = (aDot * eDot - bDot * dDot);

        if (sN < 0.0f) {
            sN = 0.0f;
            tN = eDot;
            tD = cDot;
        } else if (sN > sD) {
            sN = sD;
            tN = eDot + bDot;
            tD = cDot;
        }
    }

    if (tN < 0.0f) {
        tN = 0.0f;
        if (-dDot < 0.0f)
            sN = 0.0f;
        else if (-dDot > aDot)
            sN = sD;
        else {
            sN = -dDot;
            sD = aDot;
        }
    }

    float sc = (std::fabs(sN) < epsilon) ? 0.0f : sN / sD;
    float tc = (std::fabs(tN) < epsilon) ? 0.0f : tN / tD;

    Vector3 dP = w0 + u * sc - v * tc;
    outRayT = tc;
    return dP.dot(dP);
}
}

ExtrudeTool::ExtrudeTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

bool ExtrudeTool::computeExtrusionFrame()
{
    if (!geometry)
        return false;

    if (!profileCurve)
        return false;

    const auto& loop = profileCurve->getBoundaryLoop();
    if (loop.size() < 3)
        return false;

    Vector3 profileNormal = MeshUtils::computePolygonNormal(loop);
    if (profileNormal.lengthSquared() <= 1e-8f)
        return false;

    mode = pathCurve ? Mode::Path : Mode::Linear;
    pathClosed = false;

    if (mode == Mode::Path) {
        const auto& pathLoop = pathCurve->getBoundaryLoop();
        if (pathLoop.size() < 2)
            return false;

        Vector3 directionAggregate{ 0.0f, 0.0f, 0.0f };
        defaultDistance = 0.0f;
        for (size_t i = 1; i < pathLoop.size(); ++i) {
            Vector3 segment = pathLoop[i] - pathLoop[i - 1];
            float length = segment.length();
            if (length > kMinimumDistance) {
                directionAggregate = segment;
                defaultDistance += length;
            }
        }
        if (defaultDistance <= kMinimumDistance) {
            Vector3 span = pathLoop.back() - pathLoop.front();
            float spanLength = span.length();
            if (spanLength <= kMinimumDistance)
                return false;
            directionAggregate = span;
            defaultDistance = spanLength;
        }

        baseDirection = directionAggregate.normalized();
        if (profileNormal.lengthSquared() > 1e-8f && profileNormal.dot(baseDirection) < 0.0f)
            baseDirection = -baseDirection;
        pathClosed = loopIsClosed(pathLoop);
        previewDistance = clampDistance(defaultDistance);
    } else {
        baseDirection = profileNormal.normalized();
        previewDistance = 0.0f;
        defaultDistance = 0.0f;
    }

    anchorPoint = computeCentroid(*profileCurve);

    return true;
}

bool ExtrudeTool::projectPointerToPlane(const PointerInput& input, Vector3& out) const
{
    Vector3 origin;
    Vector3 direction;
    if (!computePointerRay(input, origin, direction))
        return false;

    float denom = direction.dot(baseDirection);
    if (std::fabs(denom) <= 1e-6f)
        return false;

    float t = (anchorPoint - origin).dot(baseDirection) / denom;
    if (t < 0.0f)
        return false;

    out = origin + direction * t;
    return true;
}

float ExtrudeTool::clampDistance(float value) const
{
    if (mode != Mode::Path)
        return value;
    if (defaultDistance <= kMinimumDistance)
        return value;
    float limit = std::max(defaultDistance * 4.0f, defaultDistance);
    if (value > limit)
        return limit;
    if (value < -limit)
        return -limit;
    return value;
}

bool ExtrudeTool::computePointerRay(const PointerInput& input, Vector3& origin, Vector3& direction) const
{
    if (!camera)
        return false;
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return false;

    if (!CameraNavigation::computeRay(*camera, input.x, input.y, viewportWidth, viewportHeight, origin, direction))
        return false;
    return true;
}

bool ExtrudeTool::pickCurveAtPointer(const PointerInput& input, Curve*& outCurve,
    Scene::Document::ObjectId& outId, bool requireFace, bool requireNoFace) const
{
    if (!geometry)
        return false;

    Vector3 rayOrigin;
    Vector3 rayDirection;
    if (!computePointerRay(input, rayOrigin, rayDirection))
        return false;

    float bestDistance = std::numeric_limits<float>::max();
    float bestRayT = std::numeric_limits<float>::max();
    bool bestHasIntersection = false;
    Curve* bestCurve = nullptr;
    Scene::Document::ObjectId bestId = 0;
    Scene::Document* doc = getDocument();
    const float selectionRadius = 0.9f;
    const float selectionRadiusSquared = selectionRadius * selectionRadius;

    for (const auto& object : geometry->getObjects()) {
        if (object->getType() != ObjectType::Curve)
            continue;
        auto* curve = static_cast<Curve*>(object.get());
        bool hasFace = !curve->getMesh().getFaces().empty();
        if (requireFace && !hasFace)
            continue;
        if (requireNoFace && hasFace)
            continue;

        bool curveHasIntersection = false;
        float curveRayT = std::numeric_limits<float>::max();

        const auto& mesh = curve->getMesh();
        const auto& vertices = mesh.getVertices();
        const auto& triangles = mesh.getTriangles();
        if (!triangles.empty()) {
            for (const auto& tri : triangles) {
                if (tri.v0 < 0 || tri.v1 < 0 || tri.v2 < 0)
                    continue;
                if (tri.v0 >= static_cast<int>(vertices.size())
                    || tri.v1 >= static_cast<int>(vertices.size())
                    || tri.v2 >= static_cast<int>(vertices.size()))
                    continue;
                float t = 0.0f;
                if (intersectRayTriangle(rayOrigin, rayDirection,
                        vertices[tri.v0].position, vertices[tri.v1].position, vertices[tri.v2].position, t)) {
                    if (t < curveRayT) {
                        curveRayT = t;
                        curveHasIntersection = true;
                    }
                }
            }
        }

        float curveDistance = std::numeric_limits<float>::max();
        float curveDistanceRayT = std::numeric_limits<float>::max();
        const auto& loop = curve->getBoundaryLoop();
        if (!loop.empty()) {
            if (loop.size() == 1) {
                float segmentRayT = 0.0f;
                float segmentDistance = closestDistanceSquaredRaySegment(rayOrigin, rayDirection, loop.front(), loop.front(), segmentRayT);
                if (segmentDistance < curveDistance && segmentRayT >= 0.0f) {
                    curveDistance = segmentDistance;
                    curveDistanceRayT = segmentRayT;
                }
            }
            for (size_t i = 0; i + 1 < loop.size(); ++i) {
                float segmentRayT = 0.0f;
                float segmentDistance = closestDistanceSquaredRaySegment(rayOrigin, rayDirection, loop[i], loop[i + 1], segmentRayT);
                if (segmentDistance < curveDistance && segmentRayT >= 0.0f) {
                    curveDistance = segmentDistance;
                    curveDistanceRayT = segmentRayT;
                }
            }
            if (loop.size() > 1 && loopIsClosed(loop)) {
                float segmentRayT = 0.0f;
                float segmentDistance = closestDistanceSquaredRaySegment(rayOrigin, rayDirection, loop.back(), loop.front(), segmentRayT);
                if (segmentDistance < curveDistance && segmentRayT >= 0.0f) {
                    curveDistance = segmentDistance;
                    curveDistanceRayT = segmentRayT;
                }
            }
        }

        bool candidateValid = false;
        bool candidateIntersection = false;
        float candidateRayT = std::numeric_limits<float>::max();
        float candidateDistance = std::numeric_limits<float>::max();

        if (curveHasIntersection) {
            candidateValid = true;
            candidateIntersection = true;
            candidateRayT = curveRayT;
            candidateDistance = 0.0f;
        } else if (curveDistance <= selectionRadiusSquared) {
            candidateValid = true;
            candidateIntersection = false;
            candidateRayT = curveDistanceRayT;
            candidateDistance = curveDistance;
        }

        if (!candidateValid)
            continue;

        Scene::Document::ObjectId candidateId = 0;
        if (doc)
            candidateId = doc->objectIdForGeometry(object.get());

        if (candidateIntersection) {
            if (!bestHasIntersection || candidateRayT < bestRayT) {
                bestHasIntersection = true;
                bestRayT = candidateRayT;
                bestDistance = candidateDistance;
                bestCurve = curve;
                bestId = candidateId;
            }
        } else if (!bestHasIntersection) {
            if (candidateDistance < bestDistance || (std::fabs(candidateDistance - bestDistance) <= 1e-6f && candidateRayT < bestRayT)) {
                bestDistance = candidateDistance;
                bestRayT = candidateRayT;
                bestCurve = curve;
                bestId = candidateId;
            }
        }
    }

    if (!bestCurve)
        return false;

    outCurve = bestCurve;
    outId = bestId;
    return true;
}

bool ExtrudeTool::trySelectProfile(const PointerInput& input)
{
    Curve* candidate = nullptr;
    Scene::Document::ObjectId id = 0;
    if (!pickCurveAtPointer(input, candidate, id, /*requireFace*/ true, /*requireNoFace*/ false))
        return false;

    profileCurve = candidate;
    profileId = id;
    pathCurve = nullptr;
    pathId = 0;
    mode = Mode::Linear;
    stage = Stage::SelectingPathOrHeight;
    dragging = false;
    if (!computeExtrusionFrame()) {
        reset();
        return false;
    }
    setState(State::Armed);
    return true;
}

bool ExtrudeTool::trySelectPath(const PointerInput& input)
{
    Curve* candidate = nullptr;
    Scene::Document::ObjectId id = 0;
    if (!pickCurveAtPointer(input, candidate, id, /*requireFace*/ false, /*requireNoFace*/ true))
        return false;
    if (!candidate || candidate == profileCurve)
        return false;

    pathCurve = candidate;
    pathId = id;
    mode = Mode::Path;
    if (!computeExtrusionFrame()) {
        pathCurve = nullptr;
        pathId = 0;
        mode = Mode::Linear;
        return false;
    }
    return true;
}

void ExtrudeTool::beginDrag(const PointerInput& input)
{
    if (!computeExtrusionFrame())
        return;

    dragging = true;
    stage = Stage::Dragging;
    setState(State::Active);
    updateDragPreview(input);
}

void ExtrudeTool::updateDragPreview(const PointerInput& input)
{
    Vector3 worldPoint;
    if (!projectPointerToPlane(input, worldPoint))
        return;
    float distance = (worldPoint - anchorPoint).dot(baseDirection);
    previewDistance = clampDistance(distance);
}

void ExtrudeTool::onPointerDown(const PointerInput& input)
{
    if (stage == Stage::SelectingProfile) {
        trySelectProfile(input);
        return;
    }

    if (stage == Stage::SelectingPathOrHeight) {
        if (trySelectPath(input))
            return;
        beginDrag(input);
        return;
    }

    if (stage == Stage::Dragging) {
        beginDrag(input);
    }
}

void ExtrudeTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;
    updateDragPreview(input);
}

void ExtrudeTool::onPointerUp(const PointerInput& input)
{
    if (!dragging)
        return;
    updateDragPreview(input);
    commit();
}

void ExtrudeTool::onCancel()
{
    dragging = false;
    reset();
}

void ExtrudeTool::onStateChanged(State /*previous*/, State next)
{
    if (next == State::Idle) {
        dragging = false;
        reset();
    }
}

void ExtrudeTool::onCommit()
{
    if (!profileCurve)
        return;

    float distance = previewDistance;
    if (std::fabs(distance) <= kMinimumDistance)
    {
        reset();
        return;
    }

    Vector3 directionVector = baseDirection * distance;
    GeometryKernel::ExtrudeOptions options;
    bool allowCaps = true;
    if (mode == Mode::Path && !pathClosed)
        allowCaps = false;
    options.capStart = allowCaps;
    options.capEnd = allowCaps;

    if (auto* stack = getCommandStack(); stack && profileId != 0) {
        std::optional<Scene::Document::ObjectId> pathOption;
        if (mode == Mode::Path && pathId != 0)
            pathOption = pathId;
        QString description = (mode == Mode::Path) ? QStringLiteral("Extrude (Path)") : QStringLiteral("Extrude (Linear)");
        auto command = std::make_unique<Tools::ExtrudeProfileCommand>(profileId, pathOption, directionVector,
            options.capStart, options.capEnd, description);
        stack->push(std::move(command));
    } else if (geometry) {
        GeometryObject* created = geometry->extrudeCurveAlongVector(static_cast<GeometryObject*>(profileCurve), directionVector, options);
        if (!created) {
            reset();
            return;
        }
    }

    reset();
}

Tool::OverrideResult ExtrudeTool::applyMeasurementOverride(double value)
{
    if (!profileCurve)
        return OverrideResult::Ignored;
    if (!computeExtrusionFrame())
        return OverrideResult::Ignored;

    previewDistance = clampDistance(static_cast<float>(value));
    dragging = false;
    stage = Stage::Dragging;
    setState(State::Active);
    return OverrideResult::Commit;
}

Tool::PreviewState ExtrudeTool::buildPreview() const
{
    PreviewState state;
    if (!profileCurve)
        return state;
    if (std::fabs(previewDistance) <= kMinimumDistance)
        return state;

    PreviewPolyline polyline;
    const auto& loop = profileCurve->getBoundaryLoop();
    polyline.closed = loop.size() > 2;
    Vector3 offset = baseDirection * previewDistance;
    for (const auto& point : loop) {
        polyline.points.push_back(point + offset);
    }
    state.polylines.push_back(std::move(polyline));
    return state;
}

void ExtrudeTool::reset()
{
    profileCurve = nullptr;
    pathCurve = nullptr;
    profileId = 0;
    pathId = 0;
    mode = Mode::None;
    stage = Stage::SelectingProfile;
    previewDistance = 0.0f;
    defaultDistance = 0.0f;
    baseDirection = Vector3(0.0f, 1.0f, 0.0f);
    pathClosed = false;
    dragging = false;
}
