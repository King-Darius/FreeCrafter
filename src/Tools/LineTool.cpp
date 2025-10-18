#include "LineTool.h"

#include "GroundProjection.h"
#include "ToolCommands.h"
#include "../Core/CommandStack.h"

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
        previewPoint = point;
        previewValid = true;
        return;
    }

    if ((point - points.back()).lengthSquared() <= kMinSegmentLengthSq) {
        previewPoint = point;
        previewValid = true;
        return;
    }

    points.push_back(point);
    previewPoint = point;
    previewValid = true;

    if (points.size() < 2)
        return;

    if (auto* stack = getCommandStack(); stack) {
        auto command = std::make_unique<Tools::CreateCurveCommand>(points, QStringLiteral("Draw Line"));
        stack->push(std::move(command));
    } else if (geometry) {
        geometry->addCurve(points);
    }

    setState(State::Idle);
    resetChain();
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
    if (geometry && points.size() >= 2)
        geometry->addCurve(points);
    setState(State::Idle);
    resetChain();
}

void LineTool::onCancel()
{
    setState(State::Idle);
    resetChain();
}

void LineTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        points.clear();
        previewValid = false;
        previewPoint = {};
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
        if (previewValid && (points.empty() || (previewPoint - points.back()).lengthSquared() > kMinSegmentLengthSq))
            polyline.points.push_back(previewPoint);
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
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool LineTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
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
    previewPoint = {};
}

Tool::OverrideResult LineTool::applyMeasurementOverride(double value)
{
    if (points.empty()) {
        const auto& snap = getInferenceResult();
        if (!snap.isValid())
            return Tool::OverrideResult::Ignored;

        setState(State::Active);
        points.push_back(snap.position);
    }

    if (points.empty())
        return Tool::OverrideResult::Ignored;

    Vector3 origin = points.back();
    Vector3 direction = previewValid ? previewPoint - origin : Vector3{};
    if (direction.lengthSquared() <= kMinSegmentLengthSq) {
        const auto& snap = getInferenceResult();
        if (snap.direction.lengthSquared() > kMinSegmentLengthSq)
            direction = snap.direction;
    }
    if (direction.lengthSquared() <= kMinSegmentLengthSq)
        return Tool::OverrideResult::Ignored;

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
