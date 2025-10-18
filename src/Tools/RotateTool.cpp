#include "RotateTool.h"

#include "GroundProjection.h"
#include "ToolCommands.h"
#include "ToolGeometryUtils.h"
#include "../Core/CommandStack.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace {
constexpr float kMinDelta = 1e-6f;

Vector3 projectOntoPlane(const Vector3& v, const Vector3& axis)
{
    return v - axis * v.dot(axis);
}
} // namespace

RotateTool::RotateTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void RotateTool::onPointerDown(const PointerInput& input)
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
    if (axis.lengthSquared() <= kMinDelta)
        axis = Vector3(0.0f, 1.0f, 0.0f);
    axis = axis.normalized();

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        dragging = false;
        return;
    }

    startVector = projectOntoPlane(world - pivot, axis);
    if (startVector.lengthSquared() <= kMinDelta)
        startVector = Vector3(1.0f, 0.0f, 0.0f);

    currentAngle = 0.0f;
    dragging = true;
    setState(State::Active);
}

void RotateTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        currentAngle = 0.0f;
        return;
    }

    Vector3 currentVector = projectOntoPlane(world - pivot, axis);
    if (currentVector.lengthSquared() <= kMinDelta || startVector.lengthSquared() <= kMinDelta) {
        currentAngle = 0.0f;
        return;
    }

    const Vector3 startNorm = startVector.normalized();
    const Vector3 currentNorm = currentVector.normalized();
    const Vector3 cross = startNorm.cross(currentNorm);
    const float sinTheta = axis.normalized().dot(cross);
    const float cosTheta = startNorm.dot(currentNorm);
    currentAngle = std::atan2(sinTheta, cosTheta);
}

void RotateTool::onPointerUp(const PointerInput& input)
{
    if (!dragging)
        return;
    onPointerMove(input);
    commit();
}

void RotateTool::onCancel()
{
    dragging = false;
    currentAngle = 0.0f;
    selection.clear();
    setState(State::Idle);
}

void RotateTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        dragging = false;
        currentAngle = 0.0f;
        selection.clear();
    }
}

void RotateTool::onCommit()
{
    if (!dragging || std::fabs(currentAngle) <= 1e-5f) {
        dragging = false;
        selection.clear();
        currentAngle = 0.0f;
        return;
    }

    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto ids = selectionIds();
        if (!ids.empty()) {
            auto command = std::make_unique<Tools::RotateObjectsCommand>(
                ids, pivot, axis, currentAngle, QStringLiteral("Rotate"));
            stack->push(std::move(command));
            executed = true;
        }
    }

    if (!executed)
        applyRotation(currentAngle);

    dragging = false;
    selection.clear();
    currentAngle = 0.0f;
}

Tool::PreviewState RotateTool::buildPreview() const
{
    PreviewState state;
    if (!dragging || selection.empty())
        return state;

    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        PreviewGhost ghost;
        ghost.object = obj;
        ghost.rotationAxis = axis;
        ghost.rotationAngle = currentAngle;
        ghost.pivot = pivot;
        ghost.usePivot = true;
        state.ghosts.push_back(ghost);
    }
    return state;
}

bool RotateTool::pointerToWorld(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    if (!camera)
        return false;
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return false;
    return ToolHelpers::screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

std::vector<GeometryObject*> RotateTool::gatherSelection() const
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

Vector3 RotateTool::determineAxis() const
{
    const auto& snap = getInferenceResult();
    if (snap.direction.lengthSquared() > kMinDelta)
        return snap.direction.normalized();
    return Vector3(0.0f, 1.0f, 0.0f);
}

Tool::OverrideResult RotateTool::applyMeasurementOverride(double value)
{
    if (!dragging || selection.empty())
        return Tool::OverrideResult::Ignored;

    currentAngle = static_cast<float>(value);
    return Tool::OverrideResult::Commit;
}

void RotateTool::applyRotation(float angleRadians)
{
    if (!geometry)
        return;
    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        rotateObject(*obj, pivot, axis, angleRadians);
    }
}

std::vector<Scene::Document::ObjectId> RotateTool::selectionIds() const
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
