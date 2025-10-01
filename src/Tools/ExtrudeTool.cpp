#include "ExtrudeTool.h"

#include "ToolGeometryUtils.h"

#include "../GeometryKernel/Curve.h"

#include <cmath>

ExtrudeTool::ExtrudeTool(GeometryKernel* geometry, CameraController* camera)
    : Tool(geometry, camera)
{
}

void ExtrudeTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    bool hasPoint = resolvePoint(input, point);

    if (!activeCurve) {
        if (!hoverCurve && hasPoint) {
            setHoverCurve(pickCurveAtPoint(point));
        }
        if (!hoverCurve) {
            return;
        }
        activeCurve = hoverCurve;
        baseLoop = activeCurve->getBoundaryLoop();
        previewHeight = 1.0f;
        updatePreview();
        setState(State::Active);
        return;
    }

    if (!previewValid) {
        updatePreview();
    }
    commit();
}

void ExtrudeTool::onPointerMove(const PointerInput& input)
{
    if (activeCurve)
        return;

    Vector3 point;
    if (!resolvePoint(input, point)) {
        setHoverCurve(nullptr);
        return;
    }
    setHoverCurve(pickCurveAtPoint(point));
}

void ExtrudeTool::onPointerHover(const PointerInput& input)
{
    onPointerMove(input);
}

void ExtrudeTool::onCommit()
{
    if (!geometry || !activeCurve || !previewValid) {
        reset();
        return;
    }
    geometry->extrudeCurve(activeCurve, previewHeight);
    reset();
}

void ExtrudeTool::onCancel()
{
    reset();
}

void ExtrudeTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (activeCurve)
        return;
    if (!result.isValid()) {
        setHoverCurve(nullptr);
        return;
    }
    setHoverCurve(pickCurveAtPoint(result.position));
}

Tool::PreviewState ExtrudeTool::buildPreview() const
{
    PreviewState state;
    if (previewValid && !previewLoop.empty()) {
        PreviewPolyline base;
        base.points = baseLoop;
        base.closed = true;
        state.polylines.push_back(base);

        PreviewPolyline top;
        top.points = previewLoop;
        top.closed = true;
        state.polylines.push_back(top);
    } else if (hoverCurve) {
        PreviewPolyline hover;
        hover.points = hoverCurve->getBoundaryLoop();
        hover.closed = true;
        state.polylines.push_back(hover);
    }
    return state;
}

Tool::OverrideResult ExtrudeTool::applyMeasurementOverride(double value)
{
    if (!activeCurve) {
        if (!hoverCurve)
            return OverrideResult::Ignored;
        activeCurve = hoverCurve;
        baseLoop = activeCurve->getBoundaryLoop();
    }
    previewHeight = static_cast<float>(value);
    updatePreview();
    return previewValid ? OverrideResult::Commit : OverrideResult::Ignored;
}

bool ExtrudeTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

Curve* ExtrudeTool::pickCurveAtPoint(const Vector3& point) const
{
    if (!geometry)
        return nullptr;
    const auto& objects = geometry->getObjects();
    for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
        GeometryObject* object = it->get();
        if (!object || object->getType() != ObjectType::Curve)
            continue;
        Curve* curve = static_cast<Curve*>(object);
        if (pointInPolygonXZ(curve->getBoundaryLoop(), point))
            return curve;
    }
    return nullptr;
}

void ExtrudeTool::setHoverCurve(Curve* curve)
{
    hoverCurve = curve;
}

void ExtrudeTool::updatePreview()
{
    if (!activeCurve) {
        previewValid = false;
        return;
    }
    baseLoop = activeCurve->getBoundaryLoop();
    previewLoop = baseLoop;
    previewValid = !previewLoop.empty();
    if (!previewValid)
        return;
    for (auto& point : previewLoop) {
        point.y += previewHeight;
    }
}

void ExtrudeTool::reset()
{
    hoverCurve = nullptr;
    activeCurve = nullptr;
    baseLoop.clear();
    previewLoop.clear();
    previewHeight = 1.0f;
    previewValid = false;
    setState(State::Idle);
}
