#include "OffsetTool.h"

#include "ToolGeometryUtils.h"

#include "../GeometryKernel/Curve.h"

#include <algorithm>
#include <cmath>

OffsetTool::OffsetTool(GeometryKernel* geometry, CameraController* camera)
    : Tool(geometry, camera)
{
}

void OffsetTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    bool hasPoint = resolvePoint(input, point);

    if (!sourceCurve) {
        if (Curve* selected = pickSelectedCurve()) {
            prepareSource(selected);
        } else if (hasPoint) {
            if (Curve* hovered = pickCurveAtPoint(point)) {
                prepareSource(hovered);
            }
        }
    }

    if (!sourceCurve)
        return;

    if (!hasFirstPoint) {
        hasFirstPoint = true;
        if (hasPoint) {
            hoverPoint = point;
            hoverValid = true;
            updatePreviewFromPoint(point);
        } else {
            updatePreview(pendingOffset);
        }
        setState(State::Active);
        return;
    }

    if (!previewValid) {
        updatePreview(pendingOffset);
    }
    commit();
}

void OffsetTool::onPointerMove(const PointerInput& input)
{
    if (!hasFirstPoint)
        return;

    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        hoverValid = false;
        return;
    }
    updatePreviewFromPoint(point);
}

void OffsetTool::onPointerHover(const PointerInput& input)
{
    if (!hasFirstPoint)
        return;

    Vector3 point;
    if (!resolvePoint(input, point)) {
        hoverValid = false;
        previewValid = false;
        return;
    }
    updatePreviewFromPoint(point);
}

void OffsetTool::onCommit()
{
    if (!geometry || !previewValid || previewLoop.empty()) {
        reset();
        return;
    }
    geometry->addCurve(previewLoop);
    reset();
}

void OffsetTool::onCancel()
{
    reset();
}

void OffsetTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!hasFirstPoint)
        return;

    if (!result.isValid()) {
        previewValid = false;
        return;
    }
    updatePreviewFromPoint(result.position);
}

Tool::PreviewState OffsetTool::buildPreview() const
{
    PreviewState state;
    if (previewValid && !previewLoop.empty()) {
        PreviewPolyline poly;
        poly.points = previewLoop;
        poly.closed = true;
        state.polylines.push_back(poly);
    } else if (sourceCurve && hoverValid) {
        PreviewPolyline base;
        base.points = baseLoop;
        base.closed = true;
        state.polylines.push_back(base);
    }
    return state;
}

Tool::OverrideResult OffsetTool::applyMeasurementOverride(double value)
{
    if (!sourceCurve || baseLoop.empty())
        return OverrideResult::Ignored;

    pendingOffset = static_cast<float>(value);
    updatePreview(pendingOffset);
    return previewValid ? OverrideResult::Commit : OverrideResult::Ignored;
}

bool OffsetTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

Curve* OffsetTool::pickSelectedCurve() const
{
    if (!geometry)
        return nullptr;
    for (const auto& object : geometry->getObjects()) {
        if (!object || !object->isSelected())
            continue;
        if (object->getType() != ObjectType::Curve)
            continue;
        return static_cast<Curve*>(object.get());
    }
    return nullptr;
}

Curve* OffsetTool::pickCurveAtPoint(const Vector3& point) const
{
    if (!geometry)
        return nullptr;
    const auto& objects = geometry->getObjects();
    for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
        GeometryObject* object = it->get();
        if (!object || object->getType() != ObjectType::Curve)
            continue;
        Curve* curve = static_cast<Curve*>(object);
        if (pointInPolygonXZ(curve->getBoundaryLoop(), point)) {
            return curve;
        }
    }
    return nullptr;
}

void OffsetTool::prepareSource(Curve* curve)
{
    if (!curve)
        return;
    sourceCurve = curve;
    baseLoop = sourceCurve->getBoundaryLoop();
    centroid = Vector3();
    if (!baseLoop.empty()) {
        for (const auto& p : baseLoop) {
            centroid += p;
        }
        centroid = centroid / static_cast<float>(baseLoop.size());
    }
    baseRadius = computeAverageRadius(baseLoop);
    pendingOffset = 1.0f;
    previewLoop.clear();
    previewValid = false;
}

void OffsetTool::updatePreview(float offsetValue)
{
    if (baseLoop.empty()) {
        previewValid = false;
        return;
    }
    previewValid = offsetPolygon(baseLoop, offsetValue, previewLoop);
    if (!previewValid)
        previewLoop.clear();
}

void OffsetTool::updatePreviewFromPoint(const Vector3& point)
{
    hoverPoint = point;
    hoverValid = true;
    Vector3 delta = point - centroid;
    float radius = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    pendingOffset = radius - baseRadius;
    updatePreview(pendingOffset);
}

void OffsetTool::reset()
{
    sourceCurve = nullptr;
    baseLoop.clear();
    previewLoop.clear();
    centroid = Vector3();
    baseRadius = 0.0f;
    pendingOffset = 1.0f;
    previewValid = false;
    hasFirstPoint = false;
    hoverValid = false;
    hoverPoint = Vector3();
    setState(State::Idle);
}

float OffsetTool::computeAverageRadius(const std::vector<Vector3>& loop) const
{
    if (loop.empty())
        return 0.0f;
    float total = 0.0f;
    for (const auto& p : loop) {
        Vector3 delta = p - centroid;
        total += std::sqrt(delta.x * delta.x + delta.z * delta.z);
    }
    return total / static_cast<float>(loop.size());
}
