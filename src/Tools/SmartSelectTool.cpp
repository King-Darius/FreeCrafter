#include "SmartSelectTool.h"

#include <QtCore/Qt>

#include <cmath>
#include <limits>
#include <memory>
#include <QString>

#include "ToolCommands.h"
#include "ToolGeometryUtils.h"
#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/HalfEdgeMesh.h"
#include "../GeometryKernel/Solid.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kPickRadius = 0.6f;

bool rayToGround(CameraController* cam, int sx, int sy, int viewportW, int viewportH, Vector3& out)
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
    const float nx = (2.0f * static_cast<float>(sx) / static_cast<float>(viewportW)) - 1.0f;
    const float ny = 1.0f - (2.0f * static_cast<float>(sy) / static_cast<float>(viewportH));
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

bool pointInsideXZ(const BoundingBox& box, float minX, float maxX, float minZ, float maxZ)
{
    return box.valid && box.min.x >= minX && box.max.x <= maxX && box.min.z >= minZ && box.max.z <= maxZ;
}

bool boxIntersectsXZ(const BoundingBox& box, float minX, float maxX, float minZ, float maxZ)
{
    if (!box.valid)
        return false;
    bool overlapX = box.max.x >= minX && box.min.x <= maxX;
    bool overlapZ = box.max.z >= minZ && box.min.z <= maxZ;
    return overlapX && overlapZ;
}
} // namespace

SmartSelectTool::SmartSelectTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void SmartSelectTool::onPointerDown(const PointerInput& input)
{
    anchorX = input.x;
    anchorY = input.y;
    currentX = input.x;
    currentY = input.y;
    dragging = false;
    rectangleValid = false;
    anchorWorldValid = pointerToWorld(input, anchorWorld);
}

void SmartSelectTool::onPointerMove(const PointerInput& input)
{
    currentX = input.x;
    currentY = input.y;
    int dx = std::abs(currentX - anchorX);
    int dy = std::abs(currentY - anchorY);
    if (!dragging && (dx > 4 || dy > 4)) {
        dragging = true;
        setState(State::Active);
    }
    if (dragging) {
        PointerInput a{ anchorX, anchorY, getModifiers() };
        PointerInput b{ currentX, currentY, getModifiers() };
        rectangleValid = pointerToWorld(a, rectStart) && pointerToWorld(b, rectEnd);
    }
}

void SmartSelectTool::onPointerUp(const PointerInput& input)
{
    currentX = input.x;
    currentY = input.y;
    if (dragging) {
        selectByRectangle(input);
    } else {
        selectSingle(input);
    }
    dragging = false;
    rectangleValid = false;
    anchorWorldValid = false;
    setState(State::Idle);
}

void SmartSelectTool::onPointerHover(const PointerInput& input)
{
    currentX = input.x;
    currentY = input.y;
}

void SmartSelectTool::onKeyDown(int key)
{
    Q_UNUSED(key);
}

void SmartSelectTool::onCancel()
{
    dragging = false;
    rectangleValid = false;
    anchorWorldValid = false;
    setState(State::Idle);
}

void SmartSelectTool::onStateChanged(State previous, State next)
{
    if (next == State::Idle) {
        dragging = false;
        rectangleValid = false;
        anchorWorldValid = false;
    }
}

Tool::PreviewState SmartSelectTool::buildPreview() const
{
    PreviewState state;
    if (dragging && rectangleValid) {
        PreviewPolyline polyline;
        Vector3 a(rectStart.x, 0.0f, rectStart.z);
        Vector3 b(rectEnd.x, 0.0f, rectStart.z);
        Vector3 c(rectEnd.x, 0.0f, rectEnd.z);
        Vector3 d(rectStart.x, 0.0f, rectEnd.z);
        polyline.points = { a, b, c, d, a };
        state.polylines.push_back(std::move(polyline));
    }
    return state;
}

GeometryObject* SmartSelectTool::pickObjectAt(const Vector3& worldPoint)
{
    if (!geometry)
        return nullptr;

    float bestDistance = std::numeric_limits<float>::max();
    GeometryObject* best = nullptr;
    const float solidBias = 0.85f;

    for (const auto& object : geometry->getObjects()) {
        if (object->getType() == ObjectType::Curve) {
            const Curve* curve = static_cast<const Curve*>(object.get());
            const auto& loop = curve->getBoundaryLoop();
            for (const auto& vertex : loop) {
                float dist = (vertex - worldPoint).lengthSquared();
                if (dist < bestDistance) {
                    bestDistance = dist;
                    best = object.get();
                }
            }
        } else {
            const HalfEdgeMesh& mesh = object->getMesh();
            float localBest = std::numeric_limits<float>::max();
            for (const auto& vertex : mesh.getVertices()) {
                float dist = (vertex.position - worldPoint).lengthSquared();
                localBest = std::min(localBest, dist);
            }
            if (localBest < std::numeric_limits<float>::max()) {
                float weighted = localBest * solidBias;
                if (weighted < bestDistance) {
                    bestDistance = weighted;
                    best = object.get();
                }
            }
        }
    }

    if (!best || bestDistance > kPickRadius * kPickRadius)
        return nullptr;
    return best;
}

bool SmartSelectTool::pointerToWorld(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return rayToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

void SmartSelectTool::applySelection(const std::vector<GeometryObject*>& hits, bool additive, bool toggle)
{
    if (!geometry)
        return;

    if (!additive && !toggle)
        clearSelection();

    for (GeometryObject* obj : hits) {
        if (!obj)
            continue;
        if (toggle) {
            obj->setSelected(!obj->isSelected());
        } else {
            obj->setSelected(true);
        }
    }
}

void SmartSelectTool::selectSingle(const PointerInput& input)
{
    Vector3 world;
    if (!pointerToWorld(input, world)) {
        if (!getModifiers().shift && !getModifiers().ctrl)
            clearSelection();
        return;
    }

    GeometryObject* candidate = pickObjectAt(world);
    bool additive = getModifiers().ctrl;
    bool toggle = getModifiers().shift;

    if (!candidate) {
        if (!additive && !toggle)
            clearSelection();
        return;
    }

    applySelection({ candidate }, additive, toggle);
}

void SmartSelectTool::selectByRectangle(const PointerInput& input)
{
    Q_UNUSED(input);

    if (!geometry || !rectangleValid)
        return;

    float minX = std::min(rectStart.x, rectEnd.x);
    float maxX = std::max(rectStart.x, rectEnd.x);
    float minZ = std::min(rectStart.z, rectEnd.z);
    float maxZ = std::max(rectStart.z, rectEnd.z);

    std::vector<GeometryObject*> hits;
    for (const auto& object : geometry->getObjects()) {
        BoundingBox box = computeBoundingBox(*object);
        if (getModifiers().shift) {
            if (boxIntersectsXZ(box, minX, maxX, minZ, maxZ))
                hits.push_back(object.get());
        } else {
            if (pointInsideXZ(box, minX, maxX, minZ, maxZ))
                hits.push_back(object.get());
        }
    }

    bool additive = getModifiers().ctrl;
    bool toggle = getModifiers().shift;
    applySelection(hits, additive, toggle);
}

void SmartSelectTool::clearSelection()
{
    if (!geometry)
        return;

    for (const auto& object : geometry->getObjects())
        object->setSelected(false);
}

}

void SmartSelectTool::clearSelection()
{
    if (!geometry)
        return;
    for (const auto& object : geometry->getObjects()) {
        object->setSelected(false);
    }
}

