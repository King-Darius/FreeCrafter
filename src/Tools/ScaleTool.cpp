#include "ScaleTool.h"

#include <cmath>
#include <algorithm>

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

Vector3 makeAxisFactors(int axisIndex, float factor)
{
    Vector3 factors(1.0f, 1.0f, 1.0f);
    if (axisIndex == 0)
        factors.x = factor;
    else if (axisIndex == 1)
        factors.y = factor;
    else
        factors.z = factor;
    return factors;
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
    for (GeometryObject* obj : selection) {
        pivot += computeCentroid(*obj);
    }
    pivot = pivot / static_cast<float>(selection.size());

    axis = determineAxis();
    axisScaling = axis.lengthSquared() > 1e-6f;
    if (axisScaling) {
        axis = axis.normalized();
    }

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
        return;
    }

    Vector3 currentVector = world - pivot;
    if (axisScaling) {
        Vector3 axisNorm = axis.normalized();
        float startComponent = startVector.dot(axisNorm);
        float currentComponent = currentVector.dot(axisNorm);
        if (std::fabs(startComponent) < 1e-5f) {
            scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
            return;
        }
        float ratio = currentComponent / startComponent;
        Vector3 absAxis(std::fabs(axisNorm.x), std::fabs(axisNorm.y), std::fabs(axisNorm.z));
        int majorAxis = 0;
        if (absAxis.y > absAxis.x && absAxis.y >= absAxis.z)
            majorAxis = 1;
        else if (absAxis.z > absAxis.x && absAxis.z >= absAxis.y)
            majorAxis = 2;
        scaleFactors = makeAxisFactors(majorAxis, ratio);
    } else {
        float startLength = startVector.length();
        if (startLength < 1e-5f) {
            scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
            return;
        }
        float currentLength = currentVector.length();
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
    dragging = false;
    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
    selection.clear();
    setState(State::Idle);
}

void ScaleTool::onStateChanged(State previous, State next)
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
    if (std::fabs(scaleFactors.x - 1.0f) <= 1e-5f && std::fabs(scaleFactors.y - 1.0f) <= 1e-5f && std::fabs(scaleFactors.z - 1.0f) <= 1e-5f) {
        dragging = false;
        selection.clear();
        scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
        return;
    }
    applyScale(scaleFactors);
    dragging = false;
    selection.clear();
    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
}

Tool::PreviewState ScaleTool::buildPreview() const
{
    PreviewState state;
    if (!dragging)
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
    if (!camera)
        return false;
    return pointerToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

std::vector<GeometryObject*> ScaleTool::gatherSelection() const
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
    if (snap.direction.lengthSquared() > 1e-6f) {
        return snap.direction.normalized();
    }
    return Vector3();
}

