#include "TransformUtils.h"

#include <cmath>

namespace GeometryTransforms {

Vector3 translate(const Vector3& point, const Vector3& delta)
{
    return point + delta;
}

Vector3 rotateAroundAxis(const Vector3& point, const Vector3& pivot, const Vector3& axis, float angleRadians)
{
    Vector3 normAxis = axis.lengthSquared() > 1e-8f ? axis.normalized() : Vector3(0.0f, 1.0f, 0.0f);
    Vector3 relative = point - pivot;
    float cosA = std::cos(angleRadians);
    float sinA = std::sin(angleRadians);
    Vector3 term1 = relative * cosA;
    Vector3 term2 = normAxis.cross(relative) * sinA;
    Vector3 term3 = normAxis * (normAxis.dot(relative) * (1.0f - cosA));
    Vector3 rotated = term1 + term2 + term3;
    return pivot + rotated;
}

Vector3 scaleFromPivot(const Vector3& point, const Vector3& pivot, const Vector3& factors)
{
    Vector3 relative = point - pivot;
    Vector3 scaled(relative.x * factors.x, relative.y * factors.y, relative.z * factors.z);
    return pivot + scaled;
}

}

