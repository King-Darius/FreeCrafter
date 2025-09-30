#include "MoveTool.h"

#include <cmath>

#include "ToolGeometryUtils.h"

namespace {

bool pointerToGround(CameraController* cam, int x, int y, int viewportW, int viewportH, Vector3& out)
{
    if (viewportW <= 0 || viewportH <= 0)
        return false;

    float cx, cy, cz;
    cam->getCameraPosition(cx, cy, cz);
    float yaw = cam->getYaw();
    float pitch = cam->getPitch();
    float ry = yaw * static_cast<float>(M_PI) / 180.0f;
    float rp = pitch * static_cast<float>(M_PI) / 180.0f;
    Vector3 forward(-sinf(ry) * cosf(rp), -sinf(rp), -cosf(ry) * cosf(rp));
    forward = forward.normalized();
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = forward.cross(up).normalized();
    up = right.cross(forward).normalized();

    const float fov = 60.0f;
    float aspect = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    float nx = (2.0f * static_cast<float>(x) / static_cast<float>(viewportW)) - 1.0f;
    float ny = 1.0f - (2.0f * static_cast<float>(y) / static_cast<float>(viewportH));
    float tanHalf = tanf((fov * static_cast<float>(M_PI) / 180.0f) / 2.0f);
    Vector3 dir = (forward + right * (nx * tanHalf * aspect) + up * (ny * tanHalf)).normalized();

    Vector3 origin(cx, cy, cz);
    if (std::fabs(dir.y) < 1e-6f)
        return false;
    float t = -origin.y / dir.y;
    if (t < 0.0f)
        return false;
    out = origin + dir * t;
    return true;
}

} // namespace

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
    applyTranslation(translation);
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
    return pointerToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
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

