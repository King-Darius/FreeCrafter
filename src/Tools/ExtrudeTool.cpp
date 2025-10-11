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
#include <optional>

namespace {
constexpr float kMinimumDistance = 1e-5f;
}

ExtrudeTool::ExtrudeTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

bool ExtrudeTool::prepareExtrusionContext()
{
    if (!geometry)
        return false;

    Scene::Document* doc = getDocument();

    profileCurve = nullptr;
    pathCurve = nullptr;
    profileId = 0;
    pathId = 0;
    mode = Mode::None;
    defaultDistance = 0.0f;

    for (const auto& object : geometry->getObjects()) {
        if (!object->isSelected())
            continue;
        if (object->getType() != ObjectType::Curve)
            continue;
        auto* curve = static_cast<Curve*>(object.get());
        bool hasFace = !curve->getMesh().getFaces().empty();
        if (hasFace) {
            if (!profileCurve) {
                profileCurve = curve;
                if (doc)
                    profileId = doc->objectIdForGeometry(object.get());
            }
        } else {
            if (!pathCurve) {
                pathCurve = curve;
                if (doc)
                    pathId = doc->objectIdForGeometry(object.get());
            }
        }
    }

    if (!profileCurve)
        return false;

    mode = pathCurve ? Mode::Path : Mode::Linear;

    const auto& loop = profileCurve->getBoundaryLoop();
    if (loop.size() < 3)
        return false;

    Vector3 profileNormal = MeshUtils::computePolygonNormal(loop);
    if (profileNormal.lengthSquared() <= 1e-8f)
        profileNormal = Vector3(0.0f, 1.0f, 0.0f);

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
    } else {
        baseDirection = profileNormal.normalized();
    }

    anchorPoint = computeCentroid(*profileCurve);
    if (mode == Mode::Path)
        previewDistance = defaultDistance;
    else
        previewDistance = 0.0f;

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

void ExtrudeTool::onPointerDown(const PointerInput& input)
{
    if (!prepareExtrusionContext())
        return;

    Vector3 worldPoint;
    if (projectPointerToPlane(input, worldPoint)) {
        float distance = (worldPoint - anchorPoint).dot(baseDirection);
        previewDistance = clampDistance(distance);
    }

    dragging = true;
    setState(State::Active);
}

void ExtrudeTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;
    Vector3 worldPoint;
    if (!projectPointerToPlane(input, worldPoint))
        return;
    float distance = (worldPoint - anchorPoint).dot(baseDirection);
    previewDistance = clampDistance(distance);
}

void ExtrudeTool::onPointerUp(const PointerInput& input)
{
    if (!dragging)
        return;
    onPointerMove(input);
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
    options.capStart = true;
    options.capEnd = true;

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
    if (!prepareExtrusionContext())
        return OverrideResult::Ignored;

    previewDistance = clampDistance(static_cast<float>(value));
    dragging = false;
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
    previewDistance = 0.0f;
    defaultDistance = 0.0f;
    baseDirection = Vector3(0.0f, 1.0f, 0.0f);
    dragging = false;
}
