#include "LineTool.h"

#include "ToolGeometryUtils.h"

LineTool::LineTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void LineTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;

    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    if (points.empty()) {
        setState(State::Active);
        points.push_back(point);
    } else {
        if ((point - points.back()).lengthSquared() > 1e-8f) {
            points.push_back(point);
        }
    }

    previewPoint = point;
    previewValid = true;
}

void LineTool::onPointerMove(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;

    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void LineTool::onPointerUp(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (points.size() >= 2) {
        previewPoint = points.back();
        previewValid = true;
    }
}

void LineTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (!points.empty()) {
        onPointerMove(input);
        return;
    }

    Vector3 point;
    if (resolvePoint(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else if (resolveFallback(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else {
        previewValid = false;
    }
}

void LineTool::onCommit()
{
    if (geometry && points.size() >= 2) {
        geometry->addCurve(points);
    }
    resetChain();
}

void LineTool::onCancel()
{
    resetChain();
}

void LineTool::onStateChanged(State previous, State next)
{
    if (next == State::Idle) {
        points.clear();
        previewValid = false;
    }
}

void LineTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (points.empty()) {
        if (result.isValid()) {
            previewPoint = result.position;
            previewValid = true;
        } else {
            Vector3 fallback;
            if (resolveFallback({ lastX, lastY, getModifiers() }, fallback)) {
                previewPoint = fallback;
                previewValid = true;
            } else {
                previewValid = false;
            }
        }
    } else if (result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState LineTool::buildPreview() const
{
    PreviewState state;
    if (!points.empty()) {
        PreviewPolyline polyline;
        polyline.points = points;
        if (previewValid && (points.empty() || (previewPoint - points.back()).lengthSquared() > 1e-8f)) {
            polyline.points.push_back(previewPoint);
        }
        state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool LineTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out)
        || resolveFallback(input, out);
}

bool LineTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!projectScreenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground)) {
        return false;
    }
    ground = snapPlanarToGrid(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void LineTool::resetChain()
{
    points.clear();
    previewValid = false;
}

Tool::OverrideResult LineTool::applyMeasurementOverride(double value)
{
    if (points.empty()) {
        const auto& snap = getInferenceResult();
        if (!snap.isValid()) {
            return Tool::OverrideResult::Ignored;
        }
        setState(State::Active);
        points.push_back(snap.position);
    }

    if (points.empty()) {
        return Tool::OverrideResult::Ignored;
    }

    Vector3 origin = points.back();
    Vector3 direction;
    if (previewValid) {
        direction = previewPoint - origin;
    }
    if (direction.lengthSquared() <= 1e-8f) {
        const auto& snap = getInferenceResult();
        if (snap.direction.lengthSquared() > 1e-8f) {
            direction = snap.direction;
        }
    }
    if (direction.lengthSquared() <= 1e-8f) {
        return Tool::OverrideResult::Ignored;
    }

    direction = direction.normalized();
    Vector3 target = origin + direction * static_cast<float>(value);
    if (points.size() == 1) {
        points.push_back(target);
    } else {
        points.back() = target;
    }
    previewPoint = target;
    previewValid = true;
    return Tool::OverrideResult::Commit;
}

