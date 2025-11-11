#include "LineTool.h"

#include "GroundProjection.h"
#include "ToolCommands.h"

#include "../Core/CommandStack.h"
#include "../Scene/Document.h"

#include <cmath>
#include <memory>
#include <QString>

using ToolHelpers::axisSnap;
using ToolHelpers::screenToGround;

namespace {
constexpr float kMinSegmentLengthSq = 1e-8f;
}

LineTool::LineTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void LineTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    if (points.empty())
        setState(State::Active);

    if (points.empty() || (point - points.back()).lengthSquared() > kMinSegmentLengthSq)
        points.push_back(point);

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
    onPointerMove(input);
}

void LineTool::onPointerHover(const PointerInput& input)
{
    if (!points.empty()) {
        onPointerMove(input);
        return;
    }

    Vector3 point;
    if (resolvePoint(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else {
        previewValid = false;
    }
}

void LineTool::onCommit()
{
    if (points.size() < 2) {
        resetChain();
        return;
    }

    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto command = std::make_unique<Tools::CreateCurveCommand>(points, QStringLiteral("Draw Line"));
        stack->push(std::move(command));
        executed = true;
    }

    if (!executed && geometry)
        geometry->addCurve(points);

    resetChain();
    setState(State::Idle);
}

void LineTool::onCancel()
{
    resetChain();
    setState(State::Idle);
}

void LineTool::onStateChanged(State previous, State next)
{
    if (next == State::Idle)
        resetChain();
}

void LineTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!points.empty()) {
        if (result.isValid()) {
            previewPoint = result.position;
            previewValid = true;
        }
        return;
    }

    if (result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
        return;
    }

    Vector3 fallback;
    if (resolveFallback({ lastX, lastY, getModifiers() }, fallback)) {
        previewPoint = fallback;
        previewValid = true;
    } else {
        previewValid = false;
    }
}

Tool::PreviewState LineTool::buildPreview() const
{
    PreviewState state;
    if (!points.empty()) {
        PreviewPolyline polyline;
        polyline.points = points;
        if (previewValid && (previewPoint - points.back()).lengthSquared() > kMinSegmentLengthSq)
            polyline.points.push_back(previewPoint);
        state.polylines.push_back(std::move(polyline));
    } else if (previewValid) {
        PreviewPolyline marker;
        marker.points.push_back(previewPoint);
        state.polylines.push_back(std::move(marker));
    }
    return state;
}

bool LineTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool LineTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    if (!camera)
        return false;
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
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
    if (value <= 0.0)
        return Tool::OverrideResult::Ignored;

    if (points.empty()) {
        const auto& snap = getInferenceResult();
        if (!snap.isValid())
            return Tool::OverrideResult::Ignored;

        setState(State::Active);
        points.push_back(snap.position);
    }

    Vector3 origin = points.back();
    Vector3 direction = previewValid ? (previewPoint - origin) : Vector3();

    if (direction.lengthSquared() <= kMinSegmentLengthSq) {
        const auto& snap = getInferenceResult();
        if (snap.direction.lengthSquared() > kMinSegmentLengthSq)
            direction = snap.direction;
    }

    if (direction.lengthSquared() <= kMinSegmentLengthSq)
        return Tool::OverrideResult::Ignored;

    direction = direction.normalized();
    Vector3 target = origin + direction * static_cast<float>(value);
    if (points.size() == 1)
        points.push_back(target);
    else
        points.back() = target;

    previewPoint = target;
    previewValid = true;
    return Tool::OverrideResult::Commit;
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

