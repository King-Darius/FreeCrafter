#include "RotateTool.h"

#include <cmath>
#include <memory>

#include "ToolCommands.h"
#include "ToolGeometryUtils.h"
#include <QString>


#include "ToolGeometryUtils.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;

bool pointerToGround(CameraController* cam, int x, int y, int viewportW, int viewportH, Vector3& out)
{
    if (viewportW <= 0 || viewportH <= 0)
        return false;

    float cx, cy, cz;
    cam->getCameraPosition(cx, cy, cz);
    float yaw = cam->getYaw();
    float pitch = cam->getPitch();
    float ry = yaw * kPi / 180.0f;
    float rp = pitch * kPi / 180.0f;
    Vector3 forward(-sinf(ry) * cosf(rp), -sinf(rp), -cosf(ry) * cosf(rp));
    forward = forward.normalized();
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = forward.cross(up).normalized();
    up = right.cross(forward).normalized();

    const float fov = 60.0f;
    float aspect = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    float nx = (2.0f * static_cast<float>(x) / static_cast<float>(viewportW)) - 1.0f;
    float ny = 1.0f - (2.0f * static_cast<float>(y) / static_cast<float>(viewportH));
    float tanHalf = tanf((fov * kPi / 180.0f) / 2.0f);
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
    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto ids = selectionIds();
        if (!ids.empty()) {
            auto command = std::make_unique<Tools::RotateObjectsCommand>(ids, pivot, axis, currentAngle, QStringLiteral("Rotate"));
            stack->push(std::move(command));
            executed = true;
        }
    }
    if (!executed) {
        applyRotation(currentAngle);
    }
    dragging = false;
    selection.clear();
    currentAngle = 0.0f;
}
Vector3 RotateTool::determineAxis() const
{
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
        pivot += computeCentroid(*obj);
    }
    pivot = pivot / static_cast<float>(selection.size());

    axis = determineAxis();
    if (axis.lengthSquared() <= 1e-6f) {
        axis = Vector3(0.0f, 1.0f, 0.0f);
    }
    axis = axis.normalized();

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        dragging = false;
        return;
    }

    startVector = projectOntoPlane(world - pivot, axis);
    if (startVector.lengthSquared() <= 1e-10f) {
        startVector = Vector3(1.0f, 0.0f, 0.0f);
    }
    dragging = true;
    currentAngle = 0.0f;
    setState(State::Active);
}

void RotateTool::onPointerMove(const PointerInput& input)
{
    if (!dragging)
        return;

    Vector3 world;
    if (!pointerToWorld(input, world)) {
        return;
    }

    Vector3 currentVector = projectOntoPlane(world - pivot, axis);
    if (currentVector.lengthSquared() <= 1e-10f || startVector.lengthSquared() <= 1e-10f) {
        currentAngle = 0.0f;
        return;
    }

    Vector3 startNorm = startVector.normalized();
    Vector3 currentNorm = currentVector.normalized();
    Vector3 cross = startNorm.cross(currentNorm);
    float sinTheta = axis.normalized().dot(cross);
    float cosTheta = startNorm.dot(currentNorm);
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
    return pointerToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

std::vector<GeometryObject*> RotateTool::gatherSelection() const
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

Vector3 RotateTool::determineAxis() const
{
    const auto& snap = getInferenceResult();
    if (snap.direction.lengthSquared() > 1e-6f) {
        return snap.direction.normalized();
    }
    return Vector3(0.0f, 1.0f, 0.0f);
}

Tool::OverrideResult RotateTool::applyMeasurementOverride(double value)
{
    if (!dragging || selection.empty()) {
        return Tool::OverrideResult::Ignored;
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

