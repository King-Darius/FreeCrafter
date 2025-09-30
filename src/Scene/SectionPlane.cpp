#include "SectionPlane.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <istream>
#include <ostream>

namespace Scene {
namespace {
constexpr float kEpsilon = 1e-5f;

Vector3 normalizeOrFallback(const Vector3& value, const Vector3& fallback)
{
    float len = value.length();
    if (len <= kEpsilon) {
        return fallback;
    }
    return value / len;
}
}

SectionPlane::SectionPlane()
{
    planeNormal = Vector3(0.0f, 1.0f, 0.0f);
    planeOrigin = Vector3(0.0f, 0.0f, 0.0f);
    transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    updateOffset();
}

void SectionPlane::setFromOriginAndNormal(const Vector3& origin, const Vector3& normal)
{
    planeOrigin = origin;
    Vector3 fallback(0.0f, 1.0f, 0.0f);
    if (std::fabs(normal.y) > 0.95f) {
        fallback = Vector3(1.0f, 0.0f, 0.0f);
    }
    planeNormal = normalizeOrFallback(normal, Vector3(0.0f, 1.0f, 0.0f));
    updateOffset();

    Vector3 xAxis = planeNormal.cross(fallback);
    if (xAxis.lengthSquared() <= kEpsilon) {
        xAxis = planeNormal.cross(Vector3(0.0f, 0.0f, 1.0f));
    }
    xAxis = normalizeOrFallback(xAxis, Vector3(1.0f, 0.0f, 0.0f));
    Vector3 yAxis = xAxis.cross(planeNormal);
    yAxis = normalizeOrFallback(yAxis, Vector3(0.0f, 0.0f, 1.0f));
    setBasis(xAxis, yAxis);
}

void SectionPlane::setBasis(const Vector3& xAxis, const Vector3& yAxis)
{
    Vector3 normal = planeNormal;
    normal = normalizeOrFallback(normal, Vector3(0.0f, 1.0f, 0.0f));
    Vector3 xNorm = normalizeOrFallback(xAxis, Vector3(1.0f, 0.0f, 0.0f));
    Vector3 yNorm = normalizeOrFallback(yAxis, Vector3(0.0f, 0.0f, 1.0f));

    transformMatrix = {
        xNorm.x, xNorm.y, xNorm.z, 0.0f,
        yNorm.x, yNorm.y, yNorm.z, 0.0f,
        normal.x, normal.y, normal.z, 0.0f,
        planeOrigin.x, planeOrigin.y, planeOrigin.z, 1.0f
    };
}

void SectionPlane::setTransform(const std::array<float, 16>& columnMajorMatrix)
{
    transformMatrix = columnMajorMatrix;
    planeOrigin = Vector3(transformMatrix[12], transformMatrix[13], transformMatrix[14]);
    Vector3 normal(transformMatrix[8], transformMatrix[9], transformMatrix[10]);
    planeNormal = normalizeOrFallback(normal, Vector3(0.0f, 1.0f, 0.0f));
    updateOffset();
}

void SectionPlane::serialize(std::ostream& os) const
{
    os << planeNormal.x << ' ' << planeNormal.y << ' ' << planeNormal.z << ' ' << planeOffset << '\n';
    os << planeOrigin.x << ' ' << planeOrigin.y << ' ' << planeOrigin.z << '\n';
    for (size_t i = 0; i < transformMatrix.size(); ++i) {
        os << transformMatrix[i];
        if ((i + 1) % 4 == 0) {
            os << '\n';
        } else {
            os << ' ';
        }
    }
    os << fill.red << ' ' << fill.green << ' ' << fill.blue << ' ' << fill.alpha << ' ';
    os << fill.extent << ' ' << (fill.fillEnabled ? 1 : 0) << ' ' << (visible ? 1 : 0) << ' ' << (active ? 1 : 0) << '\n';
}

bool SectionPlane::deserialize(std::istream& is)
{
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    float offset = 0.0f;
    float ox = 0.0f, oy = 0.0f, oz = 0.0f;
    if (!(is >> nx >> ny >> nz >> offset)) {
        return false;
    }
    if (!(is >> ox >> oy >> oz)) {
        return false;
    }
    planeNormal = normalizeOrFallback(Vector3(nx, ny, nz), Vector3(0.0f, 1.0f, 0.0f));
    planeOffset = offset;
    planeOrigin = Vector3(ox, oy, oz);

    for (float& value : transformMatrix) {
        if (!(is >> value)) {
            return false;
        }
    }

    int fillEnabled = 1;
    int visibility = 1;
    int activity = 1;
    if (!(is >> fill.red >> fill.green >> fill.blue >> fill.alpha >> fill.extent >> fillEnabled >> visibility >> activity)) {
        return false;
    }
    fill.fillEnabled = fillEnabled != 0;
    visible = visibility != 0;
    active = activity != 0;
    updateOffset();
    ensureBasis();
    return true;
}

void SectionPlane::updateOffset()
{
    planeOffset = -(planeNormal.dot(planeOrigin));
}

void SectionPlane::ensureBasis()
{
    Vector3 xAxis(transformMatrix[0], transformMatrix[1], transformMatrix[2]);
    Vector3 yAxis(transformMatrix[4], transformMatrix[5], transformMatrix[6]);
    if (xAxis.lengthSquared() <= kEpsilon || yAxis.lengthSquared() <= kEpsilon) {
        setBasis(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));
    }
}

} // namespace Scene
