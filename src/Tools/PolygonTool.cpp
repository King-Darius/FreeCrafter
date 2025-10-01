#include "PolygonTool.h"

#include "ToolGeometryUtils.h"

#include <cmath>

namespace {

std::vector<Vector3> makePolygon(const Vector3& center, const Vector3& direction, float radius, int sides)
{
    std::vector<Vector3> points;
    if (sides < 3 || radius <= 1e-4f)
        return points;

    Vector3 dir = direction;
    if (dir.lengthSquared() <= 1e-8f)
        dir = Vector3(1.0f, 0.0f, 0.0f);
    dir = dir.normalized();
    float startAngle = std::atan2(dir.z, dir.x);
    float step = 2.0f * static_cast<float>(M_PI) / static_cast<float>(sides);
    points.reserve(sides);
    for (int i = 0; i < sides; ++i) {
        float angle = startAngle + step * static_cast<float>(i);
        float x = center.x + std::cos(angle) * radius;
        float z = center.z + std::sin(angle) * radius;
        points.emplace_back(x, center.y, z);
    }
    return points;
}

}

PolygonTool::PolygonTool(GeometryKernel* geometry, CameraController* camera, int s)
    : Tool(geometry, camera)
    , sides(std::max(3, s))
{
}

Tool::OverrideResult PolygonTool::applyMeasurementOverride(double value)
{
    if (!hasCenter || !geometry)
        return OverrideResult::Ignored;

    float radius = std::fabs(static_cast<float>(value));
    if (radius <= 1e-4f)
        return OverrideResult::Ignored;

    buildPolygon(centerPoint, previewDirection, radius);
    if (!previewPolygon.empty()) {
        geometry->addCurve(previewPolygon);
        reset();
        return OverrideResult::Commit;
    }
    return OverrideResult::Ignored;
}

void PolygonTool::onPointerDown(const PointerInput& input)
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
        Vector3 direction = point - centerPoint;
        float radius = direction.length();
        if (radius > 1e-4f) {
            buildPolygon(centerPoint, direction, radius);
            if (!previewPolygon.empty() && geometry) {
                geometry->addCurve(previewPolygon);
            }
        }
        reset();
    }
}

void PolygonTool::onPointerMove(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        hoverValid = false;
        return;
    }
    updatePreview(point, true);
}

void PolygonTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        previewValid = false;
        return;
    }
    updatePreview(point, true);
}

void PolygonTool::onCommit()
{
    if (hasCenter && previewValid && geometry && !previewPolygon.empty()) {
        geometry->addCurve(previewPolygon);
    }
    reset();
}

void PolygonTool::onCancel()
{
    reset();
}

void PolygonTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
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

Tool::PreviewState PolygonTool::buildPreview() const
{
    PreviewState state;
    if (previewValid && !previewPolygon.empty()) {
        PreviewPolyline poly;
        poly.points = previewPolygon;
        poly.closed = true;
        state.polylines.push_back(poly);
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

bool PolygonTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void PolygonTool::updatePreview(const Vector3& candidate, bool valid)
{
    hoverPoint = candidate;
    hoverValid = valid;
    if (!valid) {
        previewPolygon.clear();
        previewValid = false;
        return;
    }

    if (!hasCenter) {
        previewPolygon.clear();
        previewValid = false;
        return;
    }

    Vector3 direction = candidate - centerPoint;
    float radius = direction.length();
    if (radius <= 1e-4f) {
        previewPolygon.clear();
        previewValid = false;
        return;
    }

    buildPolygon(centerPoint, direction, radius);
    previewValid = !previewPolygon.empty();
    previewDirection = direction;
}

void PolygonTool::buildPolygon(const Vector3& center, const Vector3& direction, float radius)
{
    previewPolygon = makePolygon(center, direction, radius, sides);
}

void PolygonTool::reset()
{
    hasCenter = false;
    previewValid = false;
    hoverValid = false;
    previewPolygon.clear();
    setState(State::Idle);
}

