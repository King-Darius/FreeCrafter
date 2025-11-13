#include "ScaleTool.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <QString>

#include "ToolCommands.h"
#include "ToolGeometryUtils.h"
#include "../Core/CommandStack.h"
#include "../Scene/Document.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kScaleEpsilon = 1e-5f;

bool pointerToGround(CameraController* cam, int x, int y, int viewportW, int viewportH, Vector3& out)
{
    if (!cam || viewportW <= 0 || viewportH <= 0)
        return false;

    float cx, cy, cz;
    cam->getCameraPosition(cx, cy, cz);
    const float yaw = cam->getYaw() * kPi / 180.0f;
    const float pitch = cam->getPitch() * kPi / 180.0f;

    Vector3 forward(-std::sin(yaw) * std::cos(pitch), -std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
    forward = forward.normalized();
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = forward.cross(up).normalized();
    up = right.cross(forward).normalized();

    const float fov = 60.0f;
    const float aspect = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    const float nx = (2.0f * static_cast<float>(x) / static_cast<float>(viewportW)) - 1.0f;
    const float ny = 1.0f - (2.0f * static_cast<float>(y) / static_cast<float>(viewportH));
    const float tanHalf = std::tan((fov * kPi / 180.0f) * 0.5f);
    Vector3 dir = (forward + right * (nx * tanHalf * aspect) + up * (ny * tanHalf)).normalized();

    Vector3 origin(cx, cy, cz);
    if (std::fabs(dir.y) < 1e-6f)
        return false;

    const float t = -origin.y / dir.y;
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
        Vector3 axisNorm = axis.normalized();
        float startComponent = startVector.dot(axisNorm);
        float currentComponent = currentVector.dot(axisNorm);
        if (std::fabs(startComponent) < kScaleEpsilon) {
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
    if (next == State::Idle)
        resetState();
}

void ScaleTool::onCommit()
{
    if (!dragging) {
        resetState();
        return;
    }

    if (std::fabs(scaleFactors.x - 1.0f) <= kScaleEpsilon && std::fabs(scaleFactors.y - 1.0f) <= kScaleEpsilon
        && std::fabs(scaleFactors.z - 1.0f) <= kScaleEpsilon) {
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

bool ScaleTool::pointerToWorld(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return pointerToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
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

Vector3 ScaleTool::determineAxis() const
{
    const auto& snap = getInferenceResult();
    if (snap.direction.lengthSquared() > 1e-6f)
        return snap.direction.normalized();
    return Vector3();
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
    axis = Vector3(0.0f, 1.0f, 0.0f);
    pivot = Vector3();
    startVector = Vector3();
    selection.clear();
    scaleFactors = Vector3(1.0f, 1.0f, 1.0f);
}

        else if (absAxis.z > absAxis.x && absAxis.z >= absAxis.y)
            majorAxis = 2;
        scaleFactors = makeAxisFactors(majorAxis, factor);
    } else {
        scaleFactors = Vector3(factor, factor, factor);
    }
    return Tool::OverrideResult::Commit;
}

