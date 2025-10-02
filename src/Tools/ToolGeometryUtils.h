#pragma once

#include "../GeometryKernel/GeometryObject.h"
#include "../GeometryKernel/Vector3.h"

#include <vector>

struct BoundingBox {
    Vector3 min;
    Vector3 max;
    bool valid = false;
};

BoundingBox computeBoundingBox(const GeometryObject& object);
Vector3 computeCentroid(const GeometryObject& object);
void translateObject(GeometryObject& object, const Vector3& delta);
void rotateObject(GeometryObject& object, const Vector3& pivot, const Vector3& axis, float angleRadians);
void scaleObject(GeometryObject& object, const Vector3& pivot, const Vector3& factors);

