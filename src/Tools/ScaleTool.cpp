#include "ScaleTool.h"

#include <cmath>
#include <memory>
#include <utility>

#include <QString>

#include "CameraNavigation.h"
#include "ToolCommands.h"
#include "ToolGeometryUtils.h"
#include "../Core/CommandStack.h"
#include "../Scene/Document.h"

namespace {
constexpr float kScaleEpsilon = 1e-5f;

Vector3 makeAxisFactors(int axisIndex, float factor)
{
    Vector3 result(1.0f, 1.0f, 1.0f);
    if (axisIndex == 0)
        result.x = factor;
    else if (axisIndex == 1)
        result.y = factor;
    else
        result.z = factor;
    return result;
}

int dominantAxis(const Vector3& axis)
{
    Vector3 absAxis(std::fabs(axis.x), std::fabs(axis.y), std::fabs(axis.z));
    if (absAxis.y > absAxis.x && absAxis.y >= absAxis.z)
        return 1;
    if (absAxis.z > absAxis.x && absAxis.z >= absAxis.y)
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
    axisScaling = axis.lengthSquared() > 1e-6f;
    if (axisScaling)
        axis = axis.normalized();

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        dragging = false;
        return;
    }

    startVector = world - pivot;
    if (startVector.lengthSquared() <= 1e-8f)
        startVector = axisScaling ? axis : Vector3(1.0f, 0.0f, 0.0f);

    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
    dragging = true;
    setState(State::Active);
}

void ScaleTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;

    Vector3 world;
    if (!pointerToWorld(input, world))
        return;

    Vector3 currentVector = world - pivot;
    if (axisScaling) {
        Vector3 axisNorm = axis.lengthSquared() > 1e-6f ? axis.normalized() : Vector3(1.0f, 0.0f, 0.0f);
        float startComponent = startVector.dot(axisNorm);
        float currentComponent = currentVector.dot(axisNorm);
        if (std::fabs(startComponent) < kScaleEpsilon) {
            scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
            return;
        }
        float ratio = currentComponent / startComponent;
        scaleFactors = makeAxisFactors(dominantAxis(axisNorm), ratio);
    } else {
        float startLength = startVector.length();
        float currentLength = currentVector.length();
        if (startLength < kScaleEpsilon) {
            scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
            return;
        }
        float ratio = currentLength / startLength;
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
    resetState();
    setState(State::Idle);
}

void ScaleTool::onStateChanged(State previous, State next)
{
    Q_UNUSED(previous);
    if (next == State::Idle)
        resetState();
}

void ScaleTool::onCommit()
{
    if (!dragging) {
        resetState();
        return;
    }

    const auto nearlyOne = [](float value) { return std::fabs(value - 1.0f) <= kScaleEpsilon; };
    if (nearlyOne(scaleFactors.x) && nearlyOne(scaleFactors.y) && nearlyOne(scaleFactors.z)) {
        resetState();
        return;
    }

    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto ids = selectionIds();
        if (!ids.empty()) {
            auto command = std::make_unique<Tools::ScaleObjectsCommand>(ids, pivot, scaleFactors, QStringLiteral("Scale"));
            stack->push(std::move(command));
            executed = true;
        }
    }

    if (!executed)
        applyScale(scaleFactors);

    resetState();
    setState(State::Idle);
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
        state.ghosts.push_back(std::move(ghost));
    }
    return state;
}

Tool::OverrideResult ScaleTool::applyMeasurementOverride(double value)
{
    if (!dragging || selection.empty() || value <= 0.0)
        return Tool::OverrideResult::Ignored;

    float factor = static_cast<float>(value);
    if (axisScaling) {
        scaleFactors = makeAxisFactors(dominantAxis(axis), factor);
    } else {
        scaleFactors = Vector3(factor, factor, factor);
    }
    return Tool::OverrideResult::Commit;
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

    Vector3 origin;
    Vector3 direction;
    if (!CameraNavigation::computeRay(*camera, input.x, input.y, viewportWidth, viewportHeight, origin, direction))
        return false;

    if (std::fabs(direction.y) < 1e-6f)
        return false;

    float t = -origin.y / direction.y;
    if (t < 0.0f)
        return false;

    out = origin + direction * t;
    return true;
}

std::vector<GeometryObject*> ScaleTool::gatherSelection() const
{
    std::vector<GeometryObject*> result;
    if (!geometry)
        return result;
    for (const auto& object : geometry->getObjects()) {
        if (object->isSelected())
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
    if (snap.direction.lengthSquared() > 1e-6f)
        return snap.direction.normalized();
    return Vector3();
}

std::vector<Scene::ObjectId> ScaleTool::selectionIds() const
{
    std::vector<Scene::ObjectId> ids;
    Scene::Document* doc = getDocument();
    if (!doc)
        return ids;
    ids.reserve(selection.size());
    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        Scene::ObjectId id = doc->objectIdForGeometry(obj);
        if (id != 0)
            ids.push_back(id);
    }
    return ids;
}

void ScaleTool::resetState()
{
    dragging = false;
    axisScaling = false;
    pivot = Vector3();
    axis = Vector3(0.0f, 1.0f, 0.0f);
    startVector = Vector3();
    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
    selection.clear();
}

    }

    float factor = static_cast<float>(value);
    if (axisScaling) {
        Vector3 axisNorm = axis.lengthSquared() > 1e-6f ? axis.normalized() : Vector3(1.0f, 0.0f, 0.0f);
        Vector3 absAxis(std::fabs(axisNorm.x), std::fabs(axisNorm.y), std::fabs(axisNorm.z));
        int majorAxis = 0;
        if (absAxis.y > absAxis.x && absAxis.y >= absAxis.z)
            majorAxis = 1;
        else if (absAxis.z > absAxis.x && absAxis.z >= absAxis.y)
            majorAxis = 2;
        scaleFactors = makeAxisFactors(majorAxis, factor);
    } else {
        scaleFactors = Vector3(factor, factor, factor);
    }
    return Tool::OverrideResult::Commit;
}

