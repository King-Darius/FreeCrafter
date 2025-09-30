#include "ToolGeometryUtils.h"

#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"
#include "../GeometryKernel/TransformUtils.h"
#include "../GeometryKernel/HalfEdgeMesh.h"

#include <algorithm>

namespace {

BoundingBox boxFromVertices(const std::vector<Vector3>& vertices)
{
    BoundingBox box;
    if (vertices.empty()) return box;
    box.min = vertices.front();
    box.max = vertices.front();
    box.valid = true;
    for (const auto& v : vertices) {
        box.min.x = std::min(box.min.x, v.x);
        box.min.y = std::min(box.min.y, v.y);
        box.min.z = std::min(box.min.z, v.z);
        box.max.x = std::max(box.max.x, v.x);
        box.max.y = std::max(box.max.y, v.y);
        box.max.z = std::max(box.max.z, v.z);
    }
    return box;
}

std::vector<Vector3> meshVertices(const HalfEdgeMesh& mesh)
{
    std::vector<Vector3> verts;
    verts.reserve(mesh.getVertices().size());
    for (const auto& v : mesh.getVertices()) {
        verts.push_back(v.position);
    }
    return verts;
}

}

BoundingBox computeBoundingBox(const GeometryObject& object)
{
    BoundingBox box;
    if (object.getType() == ObjectType::Curve) {
        const Curve& curve = static_cast<const Curve&>(object);
        box = boxFromVertices(curve.getBoundaryLoop());
    } else {
        const HalfEdgeMesh& mesh = object.getMesh();
        box = boxFromVertices(meshVertices(mesh));
    }
    return box;
}

Vector3 computeCentroid(const GeometryObject& object)
{
    BoundingBox box = computeBoundingBox(object);
    if (!box.valid) {
        return Vector3();
    }
    return Vector3((box.min.x + box.max.x) * 0.5f, (box.min.y + box.max.y) * 0.5f, (box.min.z + box.max.z) * 0.5f);
}

void translateObject(GeometryObject& object, const Vector3& delta)
{
    if (object.getType() == ObjectType::Curve) {
        static_cast<Curve&>(object).translate(delta);
    } else {
        static_cast<Solid&>(object).translate(delta);
    }
}

void rotateObject(GeometryObject& object, const Vector3& pivot, const Vector3& axis, float angleRadians)
{
    if (object.getType() == ObjectType::Curve) {
        static_cast<Curve&>(object).rotate(pivot, axis, angleRadians);
    } else {
        static_cast<Solid&>(object).rotate(pivot, axis, angleRadians);
    }
}

void scaleObject(GeometryObject& object, const Vector3& pivot, const Vector3& factors)
{
    if (object.getType() == ObjectType::Curve) {
        static_cast<Curve&>(object).scale(pivot, factors);
    } else {
        static_cast<Solid&>(object).scale(pivot, factors);
    }
}

