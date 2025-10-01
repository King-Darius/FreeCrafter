#include "CircleTool.h"

#include "ToolGeometryUtils.h"

#include <cmath>

namespace {

constexpr int kCircleSegments = 64;

std::vector<Vector3> makeCirclePoints(const Vector3& center, float radius, float startAngle = 0.0f)
{
    std::vector<Vector3> points;
    if (radius <= 1e-4f)
        return points;

    points.reserve(kCircleSegments);
    for (int i = 0; i < kCircleSegments; ++i) {
        float angle = startAngle + (static_cast<float>(i) / static_cast<float>(kCircleSegments)) * 2.0f * static_cast<float>(M_PI);
        float x = center.x + std::cos(angle) * radius;
        float z = center.z + std::sin(angle) * radius;
        points.emplace_back(x, center.y, z);
    }
    return points;
}

}

CircleTool::CircleTool(GeometryKernel* geometry, CameraController* camera)
    : Tool(geometry, camera)
{
}

Tool::OverrideResult CircleTool::applyMeasurementOverride(double value)
{
    if (!hasCenter || !geometry)
        return OverrideResult::Ignored;

    float radius = std::fabs(static_cast<float>(value));
    if (radius <= 1e-4f)
        return OverrideResult::Ignored;

    buildCircle(centerPoint, radius);
    if (!previewCircle.empty()) {
        geometry->addCurve(previewCircle);
        reset();
        return OverrideResult::Commit;
    }
    return OverrideResult::Ignored;
}

void CircleTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;

    if (!hasCenter) {
        centerPoint = point;
        hasCenter = true;
        setState(State::Active);
        hoverPoint = point;
        hoverValid = true;
    } else {
        float radius = (point - centerPoint).length();
        if (radius > 1e-4f && geometry) {
            buildCircle(centerPoint, radius);
            if (!previewCircle.empty()) {
                geometry->addCurve(previewCircle);
            }
        }
        reset();
    }
}

void CircleTool::onPointerMove(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        hoverValid = false;
        return;
    }
    updatePreview(point, true);
}

void CircleTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        previewValid = false;
        return;
    }
    updatePreview(point, true);
}

void CircleTool::onCommit()
{
    if (hasCenter && previewValid && geometry && !previewCircle.empty()) {
        geometry->addCurve(previewCircle);
    }
    reset();
}

void CircleTool::onCancel()
{
    reset();
}

void CircleTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (hasCenter) {
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

Tool::PreviewState CircleTool::buildPreview() const
{
    PreviewState state;
    if (previewValid && !previewCircle.empty()) {
        PreviewPolyline circle;
        circle.points = previewCircle;
        circle.closed = true;
        state.polylines.push_back(circle);
        return state;
    }

    if (hasCenter) {
        PreviewPolyline radius;
        radius.points.push_back(centerPoint);
        if (hoverValid)
            radius.points.push_back(hoverPoint);
        state.polylines.push_back(radius);
    } else if (hoverValid) {
        PreviewPolyline dot;
        dot.points.push_back(hoverPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool CircleTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void CircleTool::updatePreview(const Vector3& candidate, bool valid)
{
    hoverPoint = candidate;
    hoverValid = valid;
    if (!valid) {
        previewCircle.clear();
        previewValid = false;
        return;
    }

    if (!hasCenter) {
        previewCircle.clear();
        previewValid = false;
        return;
    }

    float radius = (candidate - centerPoint).length();
    if (radius <= 1e-4f) {
        previewCircle.clear();
        previewValid = false;
        return;
    }

    buildCircle(centerPoint, radius);
    previewValid = !previewCircle.empty();
}

void CircleTool::buildCircle(const Vector3& center, float radius)
{
    previewCircle = makeCirclePoints(center, radius, 0.0f);
}

void CircleTool::reset()
{
    hasCenter = false;
    previewValid = false;
    hoverValid = false;
    previewCircle.clear();
    setState(State::Idle);
}

