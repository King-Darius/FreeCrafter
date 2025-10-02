#include "CameraNavigation.h"

#include "CameraController.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kEpsilon = 1e-6f;

constexpr float kPi = 3.14159265358979323846f;


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

} // namespace

namespace CameraNavigation {

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

    const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

    if (camera.getProjectionMode() == CameraController::ProjectionMode::Parallel) {
        float orthoHeight = camera.getOrthoHeight();
        if (orthoHeight < kEpsilon)
            orthoHeight = 1.0f;
        float halfHeight = orthoHeight * 0.5f;
        float halfWidth = halfHeight * aspect;

        Vector3 offset = right * (nx * halfWidth) + up * (ny * halfHeight);
        origin = origin + offset;
        direction = forward;
        if (direction.lengthSquared() < kEpsilon)
            return false;
        direction = direction.normalized();
    } else {
        const float fovRadians = camera.getFieldOfView() * kPi / 180.0f;
        const float tanHalfFov = std::tan(fovRadians * 0.5f);
        direction = (forward + right * (nx * tanHalfFov * aspect) + up * (ny * tanHalfFov));
        if (direction.lengthSquared() < kEpsilon)
            return false;
        direction = direction.normalized();
    }
    return true;
}

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

    float scale = 1.0f;
    if (camera.getProjectionMode() == CameraController::ProjectionMode::Parallel) {
        float previousHeight = std::max(camera.getOrthoHeight(), 1e-3f);
        const float zoomSpeed = 1.1f;
        float factor = std::pow(zoomSpeed, -delta);
        float newHeight = std::clamp(previousHeight * factor, 0.1f, 10000.0f);
        if (std::fabs(newHeight - previousHeight) < kEpsilon)
            return false;
        camera.setOrthoHeight(newHeight);
        scale = newHeight / previousHeight;
    } else {
        float previousDistance = std::max(camera.getDistance(), 1e-3f);
        const float zoomSpeed = 1.1f;
        float factor = std::pow(zoomSpeed, -delta);
        float newDistance = std::clamp(previousDistance * factor, 0.5f, 5000.0f);
        if (std::fabs(newDistance - previousDistance) < kEpsilon)
            return false;
        camera.setDistance(newDistance);
        scale = newDistance / previousDistance;
    }

    Vector3 newCameraPos = focus + focusToCamera * scale;
    Vector3 currentTarget(tx, ty, tz);
    Vector3 focusToTarget = currentTarget - focus;
    Vector3 newTarget = focus + focusToTarget * scale;
    camera.setTarget(newTarget.x, newTarget.y, newTarget.z);
    Vector3 cameraToTarget = newCameraPos - newTarget;
    camera.setDistance(cameraToTarget.length());
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

    float aspect = (viewportWidth > 0 && viewportHeight > 0) ? static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)
                                                            : 1.0f;

    camera.setTarget(center.x, center.y, center.z);

    if (camera.getProjectionMode() == CameraController::ProjectionMode::Parallel) {
        float requiredSize = radius * 2.0f;
        float height = std::max(requiredSize, std::max(requiredSize, requiredSize / std::max(aspect, 1e-3f)));
        height = std::max(height, 0.5f);
        height *= 1.2f;
        camera.setOrthoHeight(height);
        float minDistance = radius + 0.5f;
        camera.setDistance(std::max(camera.getDistance(), minDistance));
    } else {
        const float fovRadians = camera.getFieldOfView() * kPi / 180.0f;
        const float halfFov = fovRadians * 0.5f;
        float horizontalHalfFov = std::atan(std::tan(halfFov) * aspect);

        float minDistanceVertical = radius / std::max(std::tan(halfFov), 1e-3f);
        float minDistanceHorizontal = radius / std::max(std::tan(horizontalHalfFov), 1e-3f);
        float distance = std::max({ minDistanceVertical, minDistanceHorizontal, radius + 0.5f, 1.0f });
        distance *= 1.2f;

        camera.setDistance(distance);
    }
    return true;
}

} // namespace CameraNavigation


