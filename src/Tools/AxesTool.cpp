#include "AxesTool.h"

#include "ToolGeometryUtils.h"

#include "../Scene/Document.h"

#include <cmath>

AxesTool::AxesTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* doc)
    : Tool(geometry, camera)
    , document(doc)
{
}

void AxesTool::onPointerDown(const PointerInput& input)
{
    if (!document)
        return;

    Vector3 point;
    if (!resolvePoint(input, point))
        return;

    Vector3 xAxis(1.0f, 0.0f, 0.0f);
    const auto& inference = getInferenceResult();
    if (inference.type == Interaction::InferenceSnapType::Axis && inference.direction.lengthSquared() > 1e-6f) {
        xAxis = inference.direction.normalized();
    }

    Vector3 yAxis(0.0f, 1.0f, 0.0f);
    Vector3 zAxis = xAxis.cross(yAxis);
    if (zAxis.lengthSquared() <= 1e-6f) {
        yAxis = Vector3(0.0f, 0.0f, 1.0f);
        zAxis = xAxis.cross(yAxis);
    }
    zAxis = zAxis.normalized();
    yAxis = zAxis.cross(xAxis).normalized();

    Scene::AxesGuide guide;
    guide.origin = point;
    guide.xAxis = xAxis;
    guide.yAxis = yAxis;
    guide.zAxis = zAxis;
    document->addAxesGuide(guide);
}

void AxesTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        return;
    }
    hoverPoint = point;
    hoverValid = true;
}

void AxesTool::onCancel()
{
    reset();
}

Tool::PreviewState AxesTool::buildPreview() const
{
    PreviewState state;
    if (!hoverValid)
        return state;

    PreviewPolyline xLine;
    xLine.points.push_back(hoverPoint);
    xLine.points.push_back(hoverPoint + Vector3(1.0f, 0.0f, 0.0f));
    state.polylines.push_back(xLine);

    PreviewPolyline yLine;
    yLine.points.push_back(hoverPoint);
    yLine.points.push_back(hoverPoint + Vector3(0.0f, 1.0f, 0.0f));
    state.polylines.push_back(yLine);

    PreviewPolyline zLine;
    zLine.points.push_back(hoverPoint);
    zLine.points.push_back(hoverPoint + Vector3(0.0f, 0.0f, 1.0f));
    state.polylines.push_back(zLine);

    return state;
}

bool AxesTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void AxesTool::reset()
{
    hoverValid = false;
    setState(State::Idle);
}
