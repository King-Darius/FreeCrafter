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

float distanceSquaredToSegment(const Vector3& a, const Vector3& b, const Vector3& p)
{
    Vector3 ab = b - a;
    float denom = ab.lengthSquared();
    if (denom <= 1e-12f)
        return (p - a).lengthSquared();
    float t = (p - a).dot(ab) / denom;
    t = std::clamp(t, 0.0f, 1.0f);
    Vector3 closest = a + ab * t;
    return (p - closest).lengthSquared();
}

bool loopIsClosed(const std::vector<Vector3>& loop)
{
    if (loop.size() < 3)
        return false;
    Vector3 start = loop.front();
    Vector3 end = loop.back();
    return (start - end).lengthSquared() <= (kMinimumDistance * kMinimumDistance);
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
    if (!camera)
        return false;
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return false;

    Vector3 origin;
    Vector3 direction;
    if (!CameraNavigation::computeRay(*camera, input.x, input.y, viewportWidth, viewportHeight, origin, direction))
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

bool ExtrudeTool::resolvePointerToWorld(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    if (!camera)
        return false;
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return false;

    Vector3 origin;
    Vector3 direction;
    if (!CameraNavigation::computeRay(*camera, input.x, input.y, viewportWidth, viewportHeight, origin, direction))
        return false;

    if (std::fabs(direction.y) <= 1e-6f)
        return false;
    float t = -origin.y / direction.y;
    if (t < 0.0f)
        return false;
    out = origin + direction * t;
    return true;
}

bool ExtrudeTool::pickCurveAtPointer(const PointerInput& input, Curve*& outCurve,
    Scene::Document::ObjectId& outId, bool requireFace, bool requireNoFace) const
{
    if (!geometry)
        return false;

    Vector3 worldPoint;
    if (!resolvePointerToWorld(input, worldPoint))
        return false;

    float bestDistance = std::numeric_limits<float>::max();
    Curve* bestCurve = nullptr;
    Scene::Document::ObjectId bestId = 0;
    Scene::Document* doc = getDocument();

    for (const auto& object : geometry->getObjects()) {
        if (object->getType() != ObjectType::Curve)
            continue;
        auto* curve = static_cast<Curve*>(object.get());
        bool hasFace = !curve->getMesh().getFaces().empty();
        if (requireFace && !hasFace)
            continue;
        if (requireNoFace && hasFace)
            continue;

        const auto& loop = curve->getBoundaryLoop();
        if (loop.empty())
            continue;

        float localBest = std::numeric_limits<float>::max();
        for (size_t i = 0; i < loop.size(); ++i) {
            localBest = std::min(localBest, (loop[i] - worldPoint).lengthSquared());
            if (i + 1 < loop.size())
                localBest = std::min(localBest, distanceSquaredToSegment(loop[i], loop[i + 1], worldPoint));
        }

        if (localBest < bestDistance) {
            bestDistance = localBest;
            bestCurve = curve;
            if (doc)
                bestId = doc->objectIdForGeometry(object.get());
        }
    }

    if (!bestCurve)
        return false;

    const float selectionRadius = 0.9f;
    if (bestDistance > selectionRadius * selectionRadius)
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
