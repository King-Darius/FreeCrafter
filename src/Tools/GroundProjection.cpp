#include "GroundProjection.h"

#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

namespace ToolHelpers {

bool screenToGround(CameraController* cam, int x, int y, int viewportW, int viewportH, Vector3& out)
{
    if (!cam || viewportW <= 0 || viewportH <= 0)
        return false;

    float cx, cy, cz;
    cam->getCameraPosition(cx, cy, cz);
    float yaw = cam->getYaw();
    float pitch = cam->getPitch();
    float ry = yaw * kPi / 180.0f;
    float rp = pitch * kPi / 180.0f;
    Vector3 forward(-sinf(ry) * cosf(rp), -sinf(rp), -cosf(ry) * cosf(rp));
    forward = forward.normalized();
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = forward.cross(up).normalized();
    up = right.cross(forward).normalized();

    const float fov = 60.0f;
    float aspect = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    float nx = (2.0f * static_cast<float>(x) / static_cast<float>(viewportW)) - 1.0f;
    float ny = 1.0f - (2.0f * static_cast<float>(y) / static_cast<float>(viewportH));
    float tanHalf = tanf((fov * kPi / 180.0f) / 2.0f);
    Vector3 dir = (forward + right * (nx * tanHalf * aspect) + up * (ny * tanHalf)).normalized();

    Vector3 origin(cx, cy, cz);
    if (std::fabs(dir.y) < 1e-6f)
        return false;
    float t = -origin.y / dir.y;
    if (t < 0.0f)
        return false;
    out = origin + dir * t;
    return true;
}

void axisSnap(Vector3& point)
{
    const float grid = 0.25f;
    const float epsilon = 0.08f;
    float gx = std::round(point.x / grid) * grid;
    float gz = std::round(point.z / grid) * grid;
    if (std::fabs(point.x - gx) < epsilon)
        point.x = gx;
    if (std::fabs(point.z - gz) < epsilon)
        point.z = gz;
}

}

