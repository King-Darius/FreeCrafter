#include "RotatedRectangleTool.h"

#include "ToolGeometryUtils.h"

#include <cmath>

namespace {

bool buildRectanglePoints(const std::vector<Vector3>& anchors, const Vector3& candidate, std::vector<Vector3>& out)
{
    if (anchors.size() < 2)
        return false;

    const Vector3& p0 = anchors[0];
    const Vector3& p1 = anchors[1];
    Vector3 edge = p1 - p0;
    if (edge.lengthSquared() <= 1e-8f)
        return false;

    Vector3 dir = edge.normalized();
    Vector3 perp(-dir.z, 0.0f, dir.x);
    Vector3 offset = candidate - p1;
    float width = offset.dot(perp);
    if (std::fabs(width) <= 1e-5f)
        return false;

    Vector3 height = perp * width;
    out = { p0, p1, p1 + height, p0 + height };
    return true;
}

}

RotatedRectangleTool::RotatedRectangleTool(GeometryKernel* geometry, CameraController* camera)
    : Tool(geometry, camera)
{
}

void RotatedRectangleTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;

    if (anchors.empty()) {
        anchors.push_back(point);
        hoverPoint = point;
        hoverValid = true;
        setState(State::Active);
    } else if (anchors.size() == 1) {
        if ((point - anchors.front()).lengthSquared() <= 1e-8f)
            return;
        anchors.push_back(point);
        hoverPoint = point;
        hoverValid = true;
    } else {
        std::vector<Vector3> rect;
        if (buildRectangle(point, rect) && geometry) {
            geometry->addCurve(rect);
        }
        reset();
    }
}

void RotatedRectangleTool::onPointerMove(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        hoverValid = false;
        return;
    }
    updatePreview(point, true);
}

void RotatedRectangleTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        previewValid = false;
        return;
    }
    updatePreview(point, true);
}

void RotatedRectangleTool::onCommit()
{
    if (anchors.size() == 2 && previewValid && geometry && !previewRectangle.empty()) {
        geometry->addCurve(previewRectangle);
    }
    reset();
}

void RotatedRectangleTool::onCancel()
{
    reset();
}

void RotatedRectangleTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!anchors.empty()) {
        if (result.isValid()) {
            updatePreview(result.position, true);
        } else if (hoverValid) {
            updatePreview(hoverPoint, true);
        }
    } else if (result.isValid()) {
        hoverPoint = result.position;
        hoverValid = true;
    }
}

Tool::PreviewState RotatedRectangleTool::buildPreview() const
{
    PreviewState state;
    if (previewValid && !previewRectangle.empty()) {
        PreviewPolyline rect;
        rect.points = previewRectangle;
        rect.closed = true;
        state.polylines.push_back(rect);
        return state;
    }

    if (!anchors.empty()) {
        PreviewPolyline base;
        base.points = anchors;
        if (hoverValid)
            base.points.push_back(hoverPoint);
        state.polylines.push_back(base);
    } else if (hoverValid) {
        PreviewPolyline dot;
        dot.points.push_back(hoverPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool RotatedRectangleTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void RotatedRectangleTool::updatePreview(const Vector3& candidate, bool valid)
{
    hoverPoint = candidate;
    hoverValid = valid;
    if (!valid) {
        previewRectangle.clear();
        previewValid = false;
        return;
    }

    if (anchors.size() < 1) {
        previewRectangle.clear();
        previewValid = false;
        return;
    }

    if (anchors.size() == 1) {
        previewRectangle = { anchors.front(), candidate };
        previewValid = true;
        return;
    }

    previewValid = buildRectangle(candidate, previewRectangle);
    if (!previewValid) {
        previewRectangle.clear();
    }
}

bool RotatedRectangleTool::buildRectangle(const Vector3& candidate, std::vector<Vector3>& out) const
{
    return buildRectanglePoints(anchors, candidate, out);
}

void RotatedRectangleTool::reset()
{
    anchors.clear();
    previewRectangle.clear();
    hoverValid = false;
    previewValid = false;
    setState(State::Idle);
}

