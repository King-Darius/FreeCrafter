#include "FreehandTool.h"

#include "ToolGeometryUtils.h"

FreehandTool::FreehandTool(GeometryKernel* geometry, CameraController* camera)
    : PointerDragTool(geometry, camera)
{
}

void FreehandTool::onDragStart(const PointerInput& input)
{
    stroke.clear();
    Vector3 point;
    if (samplePoint(input, point)) {
        stroke.push_back(point);
        previewStroke = stroke;
    }
}

void FreehandTool::onDragUpdate(const PointerInput& input, float, float)
{
    Vector3 point;
    if (!samplePoint(input, point))
        return;
    addPoint(point);
}

void FreehandTool::onDragEnd(const PointerInput&)
{
    if (geometry && stroke.size() >= 2) {
        geometry->addCurve(stroke);
    }
    resetStroke();
}

void FreehandTool::onDragCanceled()
{
    resetStroke();
}

void FreehandTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!samplePoint(input, point)) {
        hoverValid = false;
        return;
    }
    hoverPoint = point;
    hoverValid = true;
}

void FreehandTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (result.isValid()) {
        hoverPoint = result.position;
        hoverValid = true;
    }
}

Tool::PreviewState FreehandTool::buildPreview() const
{
    PreviewState state;
    if (!previewStroke.empty()) {
        PreviewPolyline path;
        path.points = previewStroke;
        state.polylines.push_back(path);
    } else if (hoverValid) {
        PreviewPolyline dot;
        dot.points.push_back(hoverPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool FreehandTool::samplePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void FreehandTool::addPoint(const Vector3& point)
{
    if (stroke.empty() || (stroke.back() - point).lengthSquared() > 1e-6f) {
        stroke.push_back(point);
        previewStroke = stroke;
    }
}

void FreehandTool::resetStroke()
{
    stroke.clear();
    previewStroke.clear();
    hoverValid = false;
    setState(State::Idle);
}

