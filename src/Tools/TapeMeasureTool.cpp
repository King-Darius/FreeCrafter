#include "TapeMeasureTool.h"

#include "ToolGeometryUtils.h"

#include "../Scene/Document.h"

#include <cmath>

TapeMeasureTool::TapeMeasureTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* doc)
    : Tool(geometry, camera)
    , document(doc)
{
}

void TapeMeasureTool::onPointerDown(const PointerInput& input)
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

    Scene::GuideLine guide;
    guide.start = firstPoint;
    guide.end = point;
    Vector3 delta = point - firstPoint;
    guide.length = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    document->addGuideLine(guide);
    reset();
}

void TapeMeasureTool::onPointerHover(const PointerInput& input)
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

void TapeMeasureTool::onCancel()
{
    reset();
}

Tool::PreviewState TapeMeasureTool::buildPreview() const
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

bool TapeMeasureTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void TapeMeasureTool::reset()
{
    hasFirstPoint = false;
    hoverValid = false;
    setState(State::Idle);
}
