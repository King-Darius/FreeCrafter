#pragma once

#include "Vector3.h"

namespace GeometryTransforms {

Vector3 translate(const Vector3& point, const Vector3& delta);
Vector3 rotateAroundAxis(const Vector3& point, const Vector3& pivot, const Vector3& axis, float angleRadians);
Vector3 scaleFromPivot(const Vector3& point, const Vector3& pivot, const Vector3& factors);

}

