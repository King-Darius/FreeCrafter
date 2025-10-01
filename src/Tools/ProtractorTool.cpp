#include "ProtractorTool.h"

#include "ToolGeometryUtils.h"

#include "../Scene/Document.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRadiansToDegrees = 180.0f / kPi;
}

ProtractorTool::ProtractorTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* doc)
    : Tool(geometry, camera)
    , document(doc)
{
}

void ProtractorTool::onPointerDown(const PointerInput& input)
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

    if (!hasVertexPoint) {
        vertexPoint = point;
        hasVertexPoint = true;
        hoverPoint = point;
        hoverValid = true;
        return;
    }

    if (!document) {
        reset();
        return;
    }

    Scene::AngleGuide guide;
    guide.vertex = vertexPoint;
    guide.legA = firstPoint;
    guide.legB = point;
    if (overrideAngle.has_value()) {
        guide.angle = overrideAngle.value();
    } else {
        guide.angle = computeAngle(firstPoint, vertexPoint, point);
    }
    document->addAngleGuide(guide);
    reset();
}

void ProtractorTool::onPointerHover(const PointerInput& input)
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

void ProtractorTool::onCancel()
{
    reset();
}

Tool::PreviewState ProtractorTool::buildPreview() const
{
    PreviewState state;
    if (!hasFirstPoint || !hoverValid)
        return state;

    PreviewPolyline firstLeg;
    firstLeg.points.push_back(vertexPoint);
    if (!hasVertexPoint) {
        firstLeg.points.push_back(firstPoint);
        state.polylines.push_back(firstLeg);
        return state;
    }

    firstLeg.points.clear();
    firstLeg.points.push_back(vertexPoint);
    firstLeg.points.push_back(firstPoint);
    state.polylines.push_back(firstLeg);

    PreviewPolyline secondLeg;
    secondLeg.points.push_back(vertexPoint);
    secondLeg.points.push_back(hoverPoint);
    state.polylines.push_back(secondLeg);
    return state;
}

Tool::OverrideResult ProtractorTool::applyMeasurementOverride(double value)
{
    if (!hasFirstPoint || !hasVertexPoint)
        return OverrideResult::Ignored;
    overrideAngle = static_cast<float>(value);
    return OverrideResult::PreviewUpdated;
}

bool ProtractorTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void ProtractorTool::reset()
{
    hasFirstPoint = false;
    hasVertexPoint = false;
    hoverValid = false;
    overrideAngle.reset();
    setState(State::Idle);
}

float ProtractorTool::computeAngle(const Vector3& a, const Vector3& v, const Vector3& b) const
{
    Vector3 va = a - v;
    Vector3 vb = b - v;
    float lenA = va.length();
    float lenB = vb.length();
    if (lenA <= 1e-6f || lenB <= 1e-6f)
        return 0.0f;
    va = va / lenA;
    vb = vb / lenB;
    float dot = std::clamp(va.dot(vb), -1.0f, 1.0f);
    float angle = std::acos(dot);
    return angle * kRadiansToDegrees;
}
