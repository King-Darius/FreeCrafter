#include "SmartSelectTool.h"

#include "GroundProjection.h"
#include "ToolGeometryUtils.h"

#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/HalfEdgeMesh.h"

#include <QtCore/Qt>

#include <algorithm>
#include <limits>
#include <memory>

namespace {
constexpr int kDragThreshold = 4;
constexpr float kSelectionBias = 0.8f;
constexpr float kEpsilon = 1e-6f;
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

    PointerInput anchorInput = input;
    anchorWorldValid = pointerToWorld(anchorInput, anchorWorld);
}

void SmartSelectTool::onPointerMove(const PointerInput& input)
{
    currentX = input.x;
    currentY = input.y;

    if (!dragging) {
        const int dx = std::abs(currentX - anchorX);
        const int dy = std::abs(currentY - anchorY);
        if (dx > kDragThreshold || dy > kDragThreshold) {
            dragging = true;
            setState(State::Active);
        }
    }

    if (!dragging)
        return;

    PointerInput a{ anchorX, anchorY, getModifiers(), input.devicePixelRatio };
    PointerInput b{ currentX, currentY, getModifiers(), input.devicePixelRatio };

    Vector3 start;
    Vector3 end;
    bool startValid = anchorWorldValid ? true : pointerToWorld(a, start);
    if (anchorWorldValid)
        start = anchorWorld;
    bool endValid = pointerToWorld(b, end);

    if (startValid && endValid) {
        rectStart = start;
        rectEnd = end;
        rectangleValid = true;
    } else {
        rectangleValid = false;
    }
}

void SmartSelectTool::onPointerUp(const PointerInput& input)
{
    currentX = input.x;
    currentY = input.y;

    if (dragging && rectangleValid) {
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
    Q_UNUSED(input);
}

void SmartSelectTool::onKeyDown(int key)
{
    if (key == Qt::Key_Escape) {
        cancel();
    }
}

void SmartSelectTool::onCancel()
{
    dragging = false;
    rectangleValid = false;
    anchorWorldValid = false;
    setState(State::Idle);
}

void SmartSelectTool::onStateChanged(State, State next)
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
    if (!dragging || !rectangleValid)
        return state;

    PreviewPolyline polyline;
    Vector3 a(rectStart.x, 0.0f, rectStart.z);
    Vector3 b(rectEnd.x, 0.0f, rectStart.z);
    Vector3 c(rectEnd.x, 0.0f, rectEnd.z);
    Vector3 d(rectStart.x, 0.0f, rectEnd.z);
    polyline.points = { a, b, c, d, a };
    polyline.closed = true;
    state.polylines.push_back(std::move(polyline));
    return state;
}

GeometryObject* SmartSelectTool::pickObjectAt(const Vector3& worldPoint)
{
    if (!geometry)
        return nullptr;

    GeometryObject* best = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    for (const auto& object : geometry->getObjects()) {
        if (!object || !object->isVisible() || object->isHidden())
            continue;

        float distance = std::numeric_limits<float>::max();
        if (object->getType() == ObjectType::Curve) {
            const auto* curve = static_cast<const Curve*>(object.get());
            for (const auto& vertex : curve->getBoundaryLoop())
                distance = std::min(distance, (vertex - worldPoint).lengthSquared());
        } else {
            const auto& mesh = object->getMesh();
            for (const auto& vertex : mesh.getVertices())
                distance = std::min(distance, (vertex.position - worldPoint).lengthSquared());
            distance *= kSelectionBias;
        }

        if (distance < bestDistance) {
            bestDistance = distance;
            best = object.get();
        }
    }
    return best;
}

bool SmartSelectTool::pointerToWorld(const PointerInput& input, Vector3& out) const
{
    const auto& inference = getInferenceResult();
    if (inference.isValid()) {
        out = inference.position;
        return true;
    }

    if (!camera)
        return false;
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return false;

    return ToolHelpers::screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out);
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

    if (hits.empty() && !additive && !toggle)
        clearSelection();
}

void SmartSelectTool::selectSingle(const PointerInput& input)
{
    Vector3 worldPoint;
    if (!pointerToWorld(input, worldPoint)) {
        if (!getModifiers().shift && !getModifiers().ctrl)
            clearSelection();
        return;
    }

    GeometryObject* candidate = pickObjectAt(worldPoint);
    const ModifierState mods = getModifiers();
    bool additive = mods.shift;
    bool toggle = mods.ctrl;

    if (candidate) {
        applySelection({ candidate }, additive, toggle);
    } else if (!additive && !toggle) {
        clearSelection();
    }
}

void SmartSelectTool::selectByRectangle(const PointerInput&)
{
    if (!geometry || !rectangleValid)
        return;

    const float minX = std::min(rectStart.x, rectEnd.x);
    const float maxX = std::max(rectStart.x, rectEnd.x);
    const float minZ = std::min(rectStart.z, rectEnd.z);
    const float maxZ = std::max(rectStart.z, rectEnd.z);

    const bool windowSelection = currentX >= anchorX;
    std::vector<GeometryObject*> hits;

    for (const auto& object : geometry->getObjects()) {
        if (!object || !object->isVisible() || object->isHidden())
            continue;

        BoundingBox box = computeBoundingBox(*object);
        if (!box.valid)
            continue;

        if (windowSelection) {
            const bool inside = box.min.x >= (minX - kEpsilon) && box.max.x <= (maxX + kEpsilon)
                && box.min.z >= (minZ - kEpsilon) && box.max.z <= (maxZ + kEpsilon);
            if (inside)
                hits.push_back(object.get());
        } else {
            const bool overlapX = box.max.x >= minX && box.min.x <= maxX;
            const bool overlapZ = box.max.z >= minZ && box.min.z <= maxZ;
            if (overlapX && overlapZ)
                hits.push_back(object.get());
        }
    }

    const ModifierState mods = getModifiers();
    applySelection(hits, mods.shift, mods.ctrl);
}

void SmartSelectTool::clearSelection()
{
    if (!geometry)
        return;

    for (const auto& object : geometry->getObjects()) {
        if (object)
            object->setSelected(false);
    }
}

