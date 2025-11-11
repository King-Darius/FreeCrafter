#include "RotateTool.h"

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
constexpr float kAngleEpsilon = 1e-5f;

Vector3 projectOntoPlane(const Vector3& v, const Vector3& axis)
{
    Vector3 axisNorm = axis.lengthSquared() > 1e-6f ? axis.normalized() : Vector3(0.0f, 1.0f, 0.0f);
    return v - axisNorm * v.dot(axisNorm);
}

Vector3 perpendicularTo(const Vector3& axis)
{
    Vector3 reference = std::fabs(axis.y) < 0.99f ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
    Vector3 perpendicular = axis.cross(reference);
    if (perpendicular.lengthSquared() <= 1e-6f)
        perpendicular = Vector3(1.0f, 0.0f, 0.0f);
    return perpendicular.normalized();
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
    if (axis.lengthSquared() <= 1e-6f)
        axis = Vector3(0.0f, 1.0f, 0.0f);
    axis = axis.normalized();

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        dragging = false;
        return;
    }

    startVector = projectOntoPlane(world - pivot, axis);
    if (startVector.lengthSquared() <= 1e-8f)
        startVector = perpendicularTo(axis);

    dragging = true;
    currentAngle = 0.0f;
    setState(State::Active);
}

void RotateTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;

    Vector3 world;
    if (!pointerToWorld(input, world))
        return;

    Vector3 currentVector = projectOntoPlane(world - pivot, axis);
    if (currentVector.lengthSquared() <= 1e-8f || startVector.lengthSquared() <= 1e-8f) {
        currentAngle = 0.0f;
        return;
    }

    Vector3 startNorm = startVector.normalized();
    Vector3 currentNorm = currentVector.normalized();
    Vector3 cross = startNorm.cross(currentNorm);
    const float sinTheta = axis.dot(cross);
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

void RotateTool::onStateChanged(State previous, State next)
{
    Q_UNUSED(previous);
    if (next == State::Idle) {
        dragging = false;
        currentAngle = 0.0f;
        selection.clear();
    }
}

void RotateTool::onCommit()
{
    if (!dragging || std::fabs(currentAngle) <= kAngleEpsilon) {
        dragging = false;
        selection.clear();
        currentAngle = 0.0f;
        return;
    }

    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto ids = selectionIds();
        if (!ids.empty()) {
            auto command = std::make_unique<Tools::RotateObjectsCommand>(ids, pivot, axis, currentAngle, QStringLiteral("Rotate"));
            stack->push(std::move(command));
            executed = true;
        }
    }

    if (!executed)
        applyRotation(currentAngle);

    dragging = false;
    selection.clear();
    currentAngle = 0.0f;
    setState(State::Idle);
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
        state.ghosts.push_back(std::move(ghost));
    }
    return state;
}

Tool::OverrideResult RotateTool::applyMeasurementOverride(double value)
{
    if (!dragging || selection.empty())
        return Tool::OverrideResult::Ignored;

    currentAngle = static_cast<float>(value);
    return Tool::OverrideResult::Commit;
}

bool RotateTool::pointerToWorld(const PointerInput& input, Vector3& out) const
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

    Vector3 planeNormal = axis.lengthSquared() > 1e-6f ? axis.normalized() : Vector3(0.0f, 1.0f, 0.0f);
    float denom = direction.dot(planeNormal);
    if (std::fabs(denom) < 1e-6f)
        return false;

    float t = (pivot - origin).dot(planeNormal) / denom;
    if (t < 0.0f)
        return false;

    out = origin + direction * t;
    return true;
}

std::vector<GeometryObject*> RotateTool::gatherSelection() const
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

Vector3 RotateTool::determineAxis() const
{
    const auto& snap = getInferenceResult();
    if (snap.direction.lengthSquared() > 1e-6f)
        return snap.direction.normalized();
    return Vector3(0.0f, 1.0f, 0.0f);
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

std::vector<Scene::ObjectId> RotateTool::selectionIds() const
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
    }
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

