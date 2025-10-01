#include "ZoomTool.h"

#include "../CameraNavigation.h"
#include "ToolGeometryUtils.h"

#include <algorithm>
#include <cmath>

ZoomTool::ZoomTool(GeometryKernel* g, CameraController* c)
    : PointerDragTool(g, c)
{
}

void ZoomTool::onPointerDown(const PointerInput& input)
{
    if (getModifiers().ctrl) {
        setState(State::Idle);
        zoomExtents();
        return;
    }
    if (getModifiers().alt) {
        setState(State::Idle);
        zoomSelection();
        return;
    }

    PointerDragTool::onPointerDown(input);
}

void ZoomTool::onDragStart(const PointerInput& input)
{
    (void)input;
}

void ZoomTool::onDragUpdate(const PointerInput& input, float dx, float dy)
{
    (void)dx;

    if (!camera)
        return;

    if (std::fabs(dy) < 1e-3f)
        return;

    float delta = -dy * dragSensitivity;
    if (getModifiers().shift)
        delta = -delta;

    applyZoomDelta(input, delta);
}

void ZoomTool::onDragEnd(const PointerInput& input)
{
    (void)input;
}

void ZoomTool::onDragCanceled()
{
}

void ZoomTool::applyZoomDelta(const PointerInput& input, float delta)
{
    if (!camera)
        return;
    if (!CameraNavigation::zoomAboutCursor(*camera, delta, input.x, input.y, viewportWidth, viewportHeight, zoomToCursor)) {
        camera->zoomCamera(delta);
    }
}

bool ZoomTool::zoomSelection()
{
    return zoomToBounds(true);
}

bool ZoomTool::zoomExtents()
{
    return zoomToBounds(false);
}

bool ZoomTool::zoomToBounds(bool selectedOnly)
{
    if (!geometry || !camera)
        return false;

    Vector3 minBounds;
    Vector3 maxBounds;
    bool hasBounds = false;

    for (const auto& object : geometry->getObjects()) {
        if (selectedOnly && !object->isSelected())
            continue;
        BoundingBox box = computeBoundingBox(*object);
        if (!box.valid)
            continue;
        if (!hasBounds) {
            minBounds = box.min;
            maxBounds = box.max;
            hasBounds = true;
        } else {
            minBounds.x = std::min(minBounds.x, box.min.x);
            minBounds.y = std::min(minBounds.y, box.min.y);
            minBounds.z = std::min(minBounds.z, box.min.z);
            maxBounds.x = std::max(maxBounds.x, box.max.x);
            maxBounds.y = std::max(maxBounds.y, box.max.y);
            maxBounds.z = std::max(maxBounds.z, box.max.z);
        }
    }

    if (!hasBounds)
        return false;

    return CameraNavigation::frameBounds(*camera, minBounds, maxBounds, viewportWidth, viewportHeight);
}
