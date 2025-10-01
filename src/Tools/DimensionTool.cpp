#include "DimensionTool.h"

#include "ToolGeometryUtils.h"

#include "../Scene/Document.h"

#include <cmath>

DimensionTool::DimensionTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* doc)
    : Tool(geometry, camera)
    , document(doc)
{
}

void DimensionTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;

    if (!hasFirstPoint) {
        firstPoint = point;
        hasFirstPoint = true;
        hoverPoint = point;
        hoverValid = true;
        setState(State::Active);
        return;
    }

    if (!document) {
        reset();
        return;
    }

    Scene::LinearDimension dimension;
    dimension.start = firstPoint;
    dimension.end = point;
    if (overrideDistance.has_value()) {
        dimension.value = overrideDistance.value();
    } else {
        Vector3 delta = point - firstPoint;
        dimension.value = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    }
    document->addLinearDimension(dimension);
    reset();
}

void DimensionTool::onPointerHover(const PointerInput& input)
{
    if (!hasFirstPoint)
        return;

    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        return;
    }
    hoverPoint = point;
    hoverValid = true;
}

void DimensionTool::onCancel()
{
    reset();
}

Tool::PreviewState DimensionTool::buildPreview() const
{
    PreviewState state;
    if (!hasFirstPoint || !hoverValid)
        return state;

    PreviewPolyline poly;
    poly.points.push_back(firstPoint);
    poly.points.push_back(hoverPoint);
    state.polylines.push_back(poly);
    return state;
}

Tool::OverrideResult DimensionTool::applyMeasurementOverride(double value)
{
    if (!hasFirstPoint)
        return OverrideResult::Ignored;

    overrideDistance = static_cast<float>(value);
    return OverrideResult::PreviewUpdated;
}

bool DimensionTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void DimensionTool::reset()
{
    hasFirstPoint = false;
    hoverValid = false;
    overrideDistance.reset();
    setState(State::Idle);
}
