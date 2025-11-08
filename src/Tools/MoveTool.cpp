#include "MoveTool.h"

#include <cmath>
#include <memory>

#include "CameraNavigation.h"
#include "ToolCommands.h"
#include "ToolGeometryUtils.h"

#include "../Core/CommandStack.h"
#include "../Scene/Document.h"
#include <QString>

MoveTool::MoveTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void MoveTool::onPointerDown(const PointerInput& input)
{
    selection = gatherSelection();
    if (selection.empty()) {
        dragging = false;
        return;
    }

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        dragging = false;
        return;
    }

    anchor = world;
    translation = Vector3();
    pivot = Vector3();
    for (GeometryObject* obj : selection) {
        pivot += computeCentroid(*obj);
    }
    if (!selection.empty()) {
        pivot = pivot / static_cast<float>(selection.size());
    }
    dragging = true;
    setState(State::Active);
}

void MoveTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        return;
    }

    Vector3 delta = world - anchor;
    translation = applyAxisConstraint(delta);
}

void MoveTool::onPointerUp(const PointerInput& input)
{
    if (!dragging)
        return;
    onPointerMove(input);
    commit();
}

void MoveTool::onCancel()
{
    dragging = false;
    translation = Vector3();
    selection.clear();
    setState(State::Idle);
}

void MoveTool::onStateChanged(State previous, State next)
{
    if (next == State::Idle) {
        dragging = false;
        translation = Vector3();
        selection.clear();
    }
}

void MoveTool::onCommit()
{
    if (!dragging || translation.lengthSquared() <= 1e-10f) {
        dragging = false;
        selection.clear();
        translation = Vector3();
        return;
    }
    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto ids = selectionIds();
        if (!ids.empty()) {
            auto command = std::make_unique<Tools::TranslateObjectsCommand>(ids, translation, QStringLiteral("Move"));
            stack->push(std::move(command));
            executed = true;
        }
    }
    if (!executed) {
        applyTranslation(translation);
    }
    dragging = false;
    selection.clear();
    translation = Vector3();
}

Tool::PreviewState MoveTool::buildPreview() const
{
    PreviewState state;
    if (!dragging || translation.lengthSquared() <= 1e-10f)
        return state;

    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        PreviewGhost ghost;
        ghost.object = obj;
        ghost.translation = translation;
        state.ghosts.push_back(ghost);
    }
    return state;
}

bool MoveTool::pointerToWorld(const PointerInput& input, Vector3& out) const
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

Vector3 MoveTool::applyAxisConstraint(const Vector3& delta) const
{
    const auto& snap = getInferenceResult();
    if (snap.type == Interaction::InferenceSnapType::Axis && snap.direction.lengthSquared() > 1e-6f) {
        Vector3 axis = snap.direction.normalized();
        float magnitude = delta.dot(axis);
        return axis * magnitude;
    }
    return delta;
}

Tool::OverrideResult MoveTool::applyMeasurementOverride(double value)
{
    if (!dragging || selection.empty()) {
        return Tool::OverrideResult::Ignored;
    }

    Vector3 direction = translation;
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
    translation = direction * static_cast<float>(value);
    return Tool::OverrideResult::Commit;
}

std::vector<GeometryObject*> MoveTool::gatherSelection() const
{
    std::vector<GeometryObject*> result;
    if (!geometry)
        return result;
    for (const auto& object : geometry->getObjects()) {
        if (object->isSelected()) {
            result.push_back(object.get());
        }
    }
    return result;
}

void MoveTool::applyTranslation(const Vector3& delta)
{
    if (!geometry)
        return;
    for (GeometryObject* obj : selection) {
        if (!obj)
            continue;
        translateObject(*obj, delta);
    }
}

std::vector<Scene::ObjectId> MoveTool::selectionIds() const
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

