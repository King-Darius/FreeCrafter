#include "PaintBucketTool.h"

#include "ToolGeometryUtils.h"

#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"

PaintBucketTool::PaintBucketTool(GeometryKernel* geometry, CameraController* camera)
    : Tool(geometry, camera)
{
}

void PaintBucketTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        clearHover();
        return;
    }
    updateHover(point);
    if (hoverObject) {
        hoverObject->setColor(paintColor);
    }
}

void PaintBucketTool::onPointerHover(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point)) {
        clearHover();
        return;
    }
    updateHover(point);
}

void PaintBucketTool::onPointerMove(const PointerInput& input)
{
    onPointerHover(input);
}

void PaintBucketTool::onCancel()
{
    clearHover();
    setState(State::Idle);
}

Tool::PreviewState PaintBucketTool::buildPreview() const
{
    PreviewState state;
    if (!hoverObject)
        return state;

    BoundingBox box = computeBoundingBox(*hoverObject);
    if (!box.valid)
        return state;

    PreviewPolyline poly;
    poly.closed = true;
    poly.points.push_back(Vector3(box.min.x, box.min.y, box.min.z));
    poly.points.push_back(Vector3(box.max.x, box.min.y, box.min.z));
    poly.points.push_back(Vector3(box.max.x, box.max.y, box.max.z));
    poly.points.push_back(Vector3(box.min.x, box.max.y, box.max.z));
    state.polylines.push_back(poly);
    return state;
}

bool PaintBucketTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    return resolvePlanarPoint(getInferenceResult(), camera, input.x, input.y, viewportWidth, viewportHeight, out);
}

GeometryObject* PaintBucketTool::findObjectAtPoint(const Vector3& point) const
{
    if (!geometry)
        return nullptr;

    const auto& objects = geometry->getObjects();
    for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
        GeometryObject* object = it->get();
        if (!object)
            continue;
        if (object->getType() == ObjectType::Curve) {
            Curve* curve = static_cast<Curve*>(object);
            if (pointInPolygonXZ(curve->getBoundaryLoop(), point))
                return curve;
        } else {
            BoundingBox box = computeBoundingBox(*object);
            if (!box.valid)
                continue;
            if (point.x >= box.min.x - 1e-4f && point.x <= box.max.x + 1e-4f
                && point.z >= box.min.z - 1e-4f && point.z <= box.max.z + 1e-4f) {
                return object;
            }
        }
    }
    return nullptr;
}

void PaintBucketTool::updateHover(const Vector3& point)
{
    hoverObject = findObjectAtPoint(point);
    hoverPoint = point;
    hoverValid = hoverObject != nullptr;
}

void PaintBucketTool::clearHover()
{
    hoverObject = nullptr;
    hoverPoint = Vector3();
    hoverValid = false;
}
