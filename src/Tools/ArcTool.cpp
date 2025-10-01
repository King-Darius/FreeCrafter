#include "ArcTool.h"

#include "ToolGeometryUtils.h"

#include <cmath>

namespace {

bool circleFromPoints(const Vector3& a, const Vector3& b, const Vector3& c, Vector3& center, float& radius)
{
    float x1 = a.x;
    float y1 = a.z;
    float x2 = b.x;
    float y2 = b.z;
    float x3 = c.x;
    float y3 = c.z;

    float d = 2.0f * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (std::fabs(d) < 1e-6f) {
        return false;
    }

    float ux = ((x1 * x1 + y1 * y1) * (y2 - y3) + (x2 * x2 + y2 * y2) * (y3 - y1) + (x3 * x3 + y3 * y3) * (y1 - y2)) / d;
    float uz = ((x1 * x1 + y1 * y1) * (x3 - x2) + (x2 * x2 + y2 * y2) * (x1 - x3) + (x3 * x3 + y3 * y3) * (x2 - x1)) / d;
    center = Vector3(ux, a.y, uz);
    radius = std::sqrt((ux - x1) * (ux - x1) + (uz - y1) * (uz - y1));
    return std::isfinite(radius) && radius > 1e-5f;
}

float normalizeAngle(float angle)
{
    while (angle < 0.0f)
        angle += 2.0f * static_cast<float>(M_PI);
    while (angle >= 2.0f * static_cast<float>(M_PI))
        angle -= 2.0f * static_cast<float>(M_PI);
    return angle;
}

bool buildArcPoints(const Vector3& start, const Vector3& through, const Vector3& end, std::vector<Vector3>& out)
{
    Vector3 center;
    float radius = 0.0f;
    if (!circleFromPoints(start, through, end, center, radius)) {
        out = { start, through, end };
        return false;
    }

    float startAngle = normalizeAngle(std::atan2(start.z - center.z, start.x - center.x));
    float midAngle = normalizeAngle(std::atan2(through.z - center.z, through.x - center.x));
    float endAngle = normalizeAngle(std::atan2(end.z - center.z, end.x - center.x));

    auto positiveDelta = [](float a, float b) {
        float diff = a - b;
        while (diff < 0.0f)
            diff += 2.0f * static_cast<float>(M_PI);
        while (diff >= 2.0f * static_cast<float>(M_PI))
            diff -= 2.0f * static_cast<float>(M_PI);
        return diff;
    };

    float ccwSpan = positiveDelta(endAngle, startAngle);
    float midSpan = positiveDelta(midAngle, startAngle);
    bool ccw = midSpan <= ccwSpan;
    float sweep = ccw ? ccwSpan : positiveDelta(startAngle, endAngle);

    int segments = std::max(8, static_cast<int>(std::ceil(sweep / (static_cast<float>(M_PI) / 16.0f))));
    out.clear();
    out.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float angle = ccw ? (startAngle + sweep * t) : (startAngle - sweep * t);
        float x = center.x + std::cos(angle) * radius;
        float z = center.z + std::sin(angle) * radius;
        out.emplace_back(x, start.y, z);
    }
    return true;
}

}

ArcTool::ArcTool(GeometryKernel* geometry, CameraController* camera)
    : Tool(geometry, camera)
{
}

void ArcTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        return;
    }

    if (anchors.empty()) {
        setState(State::Active);
        anchors.push_back(point);
        hoverPoint = point;
        hoverValid = true;
    } else if (anchors.size() == 1) {
        anchors.push_back(point);
        hoverPoint = point;
        hoverValid = true;
    } else {
        std::vector<Vector3> curve;
        buildArcPoints(anchors.front(), anchors[1], point, curve);
        if (!curve.empty() && geometry) {
            geometry->addCurve(curve);
        }
        reset();
    }
}

void ArcTool::onPointerMove(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        updatePreview(Vector3(), false);
        return;
    }
    updatePreview(point, true);
}

void ArcTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        previewValid = false;
        return;
    }
    updatePreview(point, true);
}

void ArcTool::onCommit()
{
    if (anchors.size() == 2 && previewValid && geometry && !previewCurve.empty()) {
        geometry->addCurve(previewCurve);
    }
    reset();
}

void ArcTool::onCancel()
{
    reset();
}

void ArcTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
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

Tool::PreviewState ArcTool::buildPreview() const
{
    PreviewState state;
    if (previewValid && !previewCurve.empty()) {
        PreviewPolyline arc;
        arc.points = previewCurve;
        state.polylines.push_back(arc);
        return state;
    }

    if (!anchors.empty()) {
        PreviewPolyline poly;
        poly.points = anchors;
        if (hoverValid) {
            poly.points.push_back(hoverPoint);
        }
        state.polylines.push_back(poly);
    } else if (hoverValid) {
        PreviewPolyline dot;
        dot.points.push_back(hoverPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool ArcTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void ArcTool::updatePreview(const Vector3& candidate, bool valid)
{
    hoverPoint = candidate;
    hoverValid = valid;

    if (!valid) {
        previewCurve.clear();
        previewValid = false;
        return;
    }

    if (anchors.empty()) {
        previewCurve.clear();
        previewValid = false;
        return;
    }

    if (anchors.size() == 1) {
        previewCurve = { anchors.front(), candidate };
        previewValid = true;
        return;
    }

    previewValid = buildArcPoints(anchors.front(), anchors[1], candidate, previewCurve);
    if (!previewValid && !previewCurve.empty()) {
        previewValid = true;
    }
}

void ArcTool::reset()
{
    anchors.clear();
    previewCurve.clear();
    hoverValid = false;
    previewValid = false;
    setState(State::Idle);
}

