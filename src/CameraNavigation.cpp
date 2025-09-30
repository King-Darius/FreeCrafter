#include "CameraNavigation.h"

#include "CameraController.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kFovDegrees = 60.0f;
constexpr float kEpsilon = 1e-6f;

Vector3 computeForward(const CameraController& camera)
{
    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);
    Vector3 cameraPos(cx, cy, cz);
    Vector3 target(tx, ty, tz);
    Vector3 forward = target - cameraPos;
    if (forward.lengthSquared() < kEpsilon)
        return Vector3(0.0f, 0.0f, -1.0f);
    return forward / forward.length();
}

bool computeRay(const CameraController& camera, int pixelX, int pixelY, int viewportWidth, int viewportHeight,
                Vector3& origin, Vector3& direction)
{
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return false;

    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    origin = Vector3(cx, cy, cz);
    Vector3 forward = computeForward(camera);
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = forward.cross(up);
    if (right.lengthSquared() < kEpsilon) {
        up = Vector3(0.0f, 0.0f, 1.0f);
        right = forward.cross(up);
    }
    right = right.lengthSquared() < kEpsilon ? Vector3(1.0f, 0.0f, 0.0f) : right.normalized();
    up = right.cross(forward).normalized();

    float nx = (2.0f * static_cast<float>(pixelX) / static_cast<float>(viewportWidth)) - 1.0f;
    float ny = 1.0f - (2.0f * static_cast<float>(pixelY) / static_cast<float>(viewportHeight));

    const float fovRadians = kFovDegrees * static_cast<float>(M_PI) / 180.0f;
    const float tanHalfFov = std::tan(fovRadians * 0.5f);
    const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

    direction = (forward + right * (nx * tanHalfFov * aspect) + up * (ny * tanHalfFov));
    if (direction.lengthSquared() < kEpsilon)
        return false;
    direction = direction.normalized();
    return true;
}

float computeNewDistance(CameraController& camera, float delta)
{
    const float zoomSpeed = 1.1f;
    float factor = std::pow(zoomSpeed, -delta);
    float distance = camera.getDistance();
    distance = std::clamp(distance * factor, 1.0f, 2000.0f);
    camera.setDistance(distance);
    return distance;
}

Vector3 offsetFromAngles(float distance, float yawDegrees, float pitchDegrees)
{
    const float yawRadians = yawDegrees * static_cast<float>(M_PI) / 180.0f;
    const float pitchRadians = pitchDegrees * static_cast<float>(M_PI) / 180.0f;
    return Vector3(std::sin(yawRadians) * std::cos(pitchRadians) * distance, std::sin(pitchRadians) * distance,
                   std::cos(yawRadians) * std::cos(pitchRadians) * distance);
}

} // namespace

namespace CameraNavigation {

bool zoomAboutCursor(CameraController& camera, float delta, int pixelX, int pixelY, int viewportWidth, int viewportHeight,
                     bool zoomToCursor)
{
    if (std::fabs(delta) < 1e-6f)
        return false;

    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    Vector3 cameraPos(cx, cy, cz);
    Vector3 target(tx, ty, tz);
    Vector3 focus = target;

    if (zoomToCursor) {
        Vector3 origin;
        Vector3 direction;
        if (computeRay(camera, pixelX, pixelY, viewportWidth, viewportHeight, origin, direction)) {
            Vector3 planeNormal = target - cameraPos;
            float denom = planeNormal.dot(direction);
            if (std::fabs(denom) > kEpsilon) {
                float t = planeNormal.dot(target - origin) / denom;
                if (t > 0.0f)
                    focus = origin + direction * t;
            }
        }
    }

    Vector3 focusToCamera = cameraPos - focus;
    if (focusToCamera.lengthSquared() < kEpsilon)
        focusToCamera = cameraPos - target;

    if (focusToCamera.lengthSquared() < kEpsilon)
        focusToCamera = Vector3(0.0f, 0.0f, 1.0f);

    float previousDistance = camera.getDistance();
    float newDistance = computeNewDistance(camera, delta);
    if (std::fabs(previousDistance) < kEpsilon)
        previousDistance = 1.0f;
    float scale = newDistance / previousDistance;
    Vector3 newCameraPos = focus + focusToCamera * scale;

    Vector3 offset = offsetFromAngles(newDistance, camera.getYaw(), camera.getPitch());
    Vector3 newTarget = newCameraPos - offset;
    camera.setTarget(newTarget.x, newTarget.y, newTarget.z);
    return true;
}

bool frameBounds(CameraController& camera, const Vector3& minBounds, const Vector3& maxBounds, int viewportWidth,
                 int viewportHeight)
{
    Vector3 center = (minBounds + maxBounds) * 0.5f;
    Vector3 extents = (maxBounds - minBounds) * 0.5f;
    float radius = extents.length();
    float largestComponent = std::max({ std::fabs(extents.x), std::fabs(extents.y), std::fabs(extents.z) });
    radius = std::max(radius, largestComponent);
    if (radius < 1e-3f)
        radius = std::max(largestComponent, 0.5f);

    const float fovRadians = kFovDegrees * static_cast<float>(M_PI) / 180.0f;
    const float halfFov = fovRadians * 0.5f;
    float aspect = (viewportWidth > 0 && viewportHeight > 0) ? static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)
                                                            : 1.0f;
    float horizontalHalfFov = std::atan(std::tan(halfFov) * aspect);

    float minDistanceVertical = radius / std::max(std::tan(halfFov), 1e-3f);
    float minDistanceHorizontal = radius / std::max(std::tan(horizontalHalfFov), 1e-3f);
    float distance = std::max({ minDistanceVertical, minDistanceHorizontal, radius + 0.5f, 1.0f });
    distance *= 1.2f;

    camera.setTarget(center.x, center.y, center.z);
    camera.setDistance(distance);
    return true;
}

} // namespace CameraNavigation
