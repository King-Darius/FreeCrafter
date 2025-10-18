#include "ScaleTool.h"

#include "GroundProjection.h"
#include "ToolCommands.h"
#include "ToolGeometryUtils.h"
#include "../Core/CommandStack.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace {
constexpr float kMinComponent = 1e-6f;

Vector3 axisFactors(int axisIndex, float factor)
{
    Vector3 result(1.0f, 1.0f, 1.0f);
    switch (axisIndex) {
    case 0:
        result.x = factor;
        break;
    case 1:
        result.y = factor;
        break;
    default:
        result.z = factor;
        break;
    }
    return result;
}

int dominantAxis(const Vector3& dir)
{
    Vector3 absDir(std::fabs(dir.x), std::fabs(dir.y), std::fabs(dir.z));
    if (absDir.y >= absDir.x && absDir.y >= absDir.z)
        return 1;
    if (absDir.z >= absDir.x && absDir.z >= absDir.y)
        return 2;
    return 0;
}
} // namespace

ScaleTool::ScaleTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void ScaleTool::onPointerDown(const PointerInput& input)
{
    selection = gatherSelection();
    if (selection.empty()) {
        dragging = false;
        return;
    }

    pivot = Vector3();
    for (GeometryObject* obj : selection)
        pivot += computeCentroid(*obj);
    pivot /= static_cast<float>(selection.size());

    axis = determineAxis();
    axisScaling = axis.lengthSquared() > kMinComponent;
    if (axisScaling)
        axis = axis.normalized();

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        dragging = false;
        return;
    }

    startVector = world - pivot;
    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
    dragging = true;
    setState(State::Active);
}

void ScaleTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
        return;
    }

    Vector3 currentVector = world - pivot;
    if (axisScaling) {
        const float startComponent = startVector.dot(axis);
        if (std::fabs(startComponent) <= kMinComponent) {
            scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
            return;
        }
        const float ratio = currentVector.dot(axis) / startComponent;
        scaleFactors = axisFactors(dominantAxis(axis), ratio);
    } else {
        const float startLength = startVector.length();
        if (startLength <= kMinComponent) {
            scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
            return;
        }
        const float ratio = currentVector.length() / startLength;
        scaleFactors = Vector3(ratio, ratio, ratio);
    }
}

void ScaleTool::onPointerUp(const PointerInput& input)
{
    if (!dragging)
        return;
    onPointerMove(input);
    commit();
}

void ScaleTool::onCancel()
{
    dragging = false;
    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
    selection.clear();
    setState(State::Idle);
}

void ScaleTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        dragging = false;
        scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
        selection.clear();
    }
}

void ScaleTool::onCommit()
{
    if (!dragging) {
        selection.clear();
        scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
        return;
    }

    const Vector3 delta = scaleFactors - Vector3(1.0f, 1.0f, 1.0f);
    if (std::fabs(delta.x) <= 1e-5f && std::fabs(delta.y) <= 1e-5f && std::fabs(delta.z) <= 1e-5f) {
        dragging = false;
        selection.clear();
        scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
        return;
    }

    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto ids = selectionIds();
        if (!ids.empty()) {
            auto command = std::make_unique<Tools::ScaleObjectsCommand>(
                ids, pivot, scaleFactors, QStringLiteral("Scale"));
            stack->push(std::move(command));
            executed = true;
        }
    }

    if (!executed)
        applyScale(scaleFactors);

    dragging = false;
    selection.clear();
    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
}

Tool::PreviewState ScaleTool::buildPreview() const
{
    PreviewState state;
    if (!dragging || selection.empty())
        return state;

    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        PreviewGhost ghost;
        ghost.object = obj;
        ghost.scale = scaleFactors;
        ghost.pivot = pivot;
        ghost.usePivot = true;
        state.ghosts.push_back(ghost);
    }
    return state;
}

bool ScaleTool::pointerToWorld(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    if (!camera || viewportWidth <= 0 || viewportHeight <= 0)
        return false;
    return ToolHelpers::screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

std::vector<GeometryObject*> ScaleTool::gatherSelection() const
{
    std::vector<GeometryObject*> result;
    if (!geometry)
        return result;
    for (const auto& object : geometry->getObjects()) {
        if (object && object->isSelected())
            result.push_back(object.get());
    }
    return result;
}

void ScaleTool::applyScale(const Vector3& factors)
{
    if (!geometry)
        return;
    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        scaleObject(*obj, pivot, factors);
    }
}

Vector3 ScaleTool::determineAxis() const
{
    const auto& snap = getInferenceResult();
    if (snap.direction.lengthSquared() > kMinComponent)
        return snap.direction.normalized();
    return Vector3();
}

Tool::OverrideResult ScaleTool::applyMeasurementOverride(double value)
{
    if (!dragging || selection.empty() || value <= 0.0)
        return Tool::OverrideResult::Ignored;

    const float factor = static_cast<float>(value);
    if (axisScaling) {
        scaleFactors = axisFactors(dominantAxis(axis), factor);
    } else {
        scaleFactors = Vector3(factor, factor, factor);
    }
    return Tool::OverrideResult::Commit;
}

std::vector<Scene::Document::ObjectId> ScaleTool::selectionIds() const
{
    std::vector<Scene::Document::ObjectId> ids;
    Scene::Document* doc = getDocument();
    if (!doc)
        return ids;

    ids.reserve(selection.size());
    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        Scene::Document::ObjectId id = doc->objectIdForGeometry(obj);
        if (id != 0)
            ids.push_back(id);
    }
    return ids;
}
