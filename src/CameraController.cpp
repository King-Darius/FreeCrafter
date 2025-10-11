#include "CameraController.h"

#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

constexpr float kMinDistance = 0.5f;
constexpr float kMaxDistance = 5000.0f;
constexpr float kMinFov = 15.0f;
constexpr float kMaxFov = 120.0f;
constexpr float kMinOrthoHeight = 0.1f;
constexpr float kMaxOrthoHeight = 10000.0f;

float clampFov(float value)
{
    if (value < kMinFov)
        return kMinFov;
    if (value > kMaxFov)
        return kMaxFov;
    return value;
}

}

CameraController::CameraController()
    : yaw(45.0f)
    , pitch(35.0f)
    , distance(20.0f)
    , targetX(0.0f)
    , targetY(0.0f)
    , targetZ(0.0f)
    , projectionMode(ProjectionMode::Perspective)
    , fieldOfView(60.0f)
    , orthoHeight(orthoHeightFromPerspectiveDistance())
{}

void CameraController::getCameraPosition(float& x, float& y, float& z) const {
    float radYaw = yaw * (float)M_PI / 180.0f;
    float radPitch = pitch * (float)M_PI / 180.0f;
    float cx = distance * sin(radYaw) * cos(radPitch);
    float cy = distance * sin(radPitch);
    float cz = distance * cos(radYaw) * cos(radPitch);
    x = targetX + cx; y = targetY + cy; z = targetZ + cz;
}

void CameraController::rotateCamera(float dx, float dy) {
    const float rotateSpeed = 0.3f;
    yaw += dx * rotateSpeed;
    pitch += dy * rotateSpeed;
    normalizeYaw();
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void CameraController::panCamera(float dx, float dy) {
    float radYaw = yaw * (float)M_PI / 180.0f;
    float radPitch = pitch * (float)M_PI / 180.0f;

    float cosPitch = std::cos(radPitch);
    float sinPitch = std::sin(radPitch);
    float sinYaw = std::sin(radYaw);
    float cosYaw = std::cos(radYaw);

    // View direction from camera toward the target
    float dirX = -sinYaw * cosPitch;
    float dirY = -sinPitch;
    float dirZ = -cosYaw * cosPitch;

    float dirLen = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (dirLen > 1e-5f) {
        dirX /= dirLen;
        dirY /= dirLen;
        dirZ /= dirLen;
    }

    // Right vector (perpendicular to world up)
    float rightX = dirZ;
    float rightY = 0.0f;
    float rightZ = -dirX;
    float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
    if (rightLen > 1e-5f) {
        rightX /= rightLen;
        rightY /= rightLen;
        rightZ /= rightLen;
    } else {
        rightX = 1.0f;
        rightY = 0.0f;
        rightZ = 0.0f;
    }

    // True camera up vector derived from view and right axes
    float upX = rightY * dirZ - rightZ * dirY;
    float upY = rightZ * dirX - rightX * dirZ;
    float upZ = rightX * dirY - rightY * dirX;
    float upLen = std::sqrt(upX * upX + upY * upY + upZ * upZ);
    if (upLen > 1e-5f) {
        upX /= upLen;
        upY /= upLen;
        upZ /= upLen;
    } else {
        upX = 0.0f;
        upY = 1.0f;
        upZ = 0.0f;
    }

    float reference = projectionMode == ProjectionMode::Parallel ? orthoHeight : distance;
    if (reference <= 0.0f)
        reference = distance;
    if (reference <= 0.0f)
        reference = 1.0f;
    const float panSpeed = 0.0025f * reference;
    targetX -= (rightX * dx + upX * dy) * panSpeed;
    targetY -= (rightY * dx + upY * dy) * panSpeed;
    targetZ -= (rightZ * dx + upZ * dy) * panSpeed;
}

void CameraController::zoomCamera(float delta) {
    const float zoomSpeed = 1.1f;
    float factor = std::pow(zoomSpeed, -delta);
    if (projectionMode == ProjectionMode::Parallel) {
        orthoHeight *= factor;
        if (orthoHeight < kMinOrthoHeight) orthoHeight = kMinOrthoHeight;
        if (orthoHeight > kMaxOrthoHeight) orthoHeight = kMaxOrthoHeight;
        distance *= factor;
        if (distance < kMinDistance) distance = kMinDistance;
        if (distance > kMaxDistance) distance = kMaxDistance;
    } else {
        distance *= factor;
        if (distance < kMinDistance) distance = kMinDistance;
        if (distance > kMaxDistance) distance = kMaxDistance;
        orthoHeight = orthoHeightFromPerspectiveDistance();
    }
}

void CameraController::setTarget(float tx, float ty, float tz) { targetX = tx; targetY = ty; targetZ = tz; }
void CameraController::setDistance(float dist) {
    distance = std::clamp(dist, kMinDistance, kMaxDistance);
    if (projectionMode == ProjectionMode::Perspective)
        orthoHeight = orthoHeightFromPerspectiveDistance();
}
void CameraController::setYaw(float newYaw) {
    yaw = newYaw;
    normalizeYaw();
}
void CameraController::setPitch(float newPitch) {
    pitch = newPitch;
    clampPitch();
}
void CameraController::setYawPitch(float newYaw, float newPitch) {
    yaw = newYaw;
    pitch = newPitch;
    normalizeYaw();
    clampPitch();
}
float CameraController::getYaw() const { return yaw; }
float CameraController::getPitch() const { return pitch; }
float CameraController::getDistance() const { return distance; }
void CameraController::getTarget(float& tx, float& ty, float& tz) const { tx = targetX; ty = targetY; tz = targetZ; }
float CameraController::getFieldOfView() const { return fieldOfView; }
void CameraController::setFieldOfView(float degrees) {
    fieldOfView = clampFov(degrees);
    if (projectionMode == ProjectionMode::Perspective)
        orthoHeight = orthoHeightFromPerspectiveDistance();
}
float CameraController::getOrthoHeight() const { return orthoHeight; }
void CameraController::setOrthoHeight(float height) {
    orthoHeight = std::clamp(height, kMinOrthoHeight, kMaxOrthoHeight);
    if (projectionMode == ProjectionMode::Parallel)
        distance = std::clamp(perspectiveDistanceFromOrthoHeight(), kMinDistance, kMaxDistance);
}
CameraController::ProjectionMode CameraController::getProjectionMode() const { return projectionMode; }
void CameraController::setProjectionMode(ProjectionMode mode) {
    if (projectionMode == mode)
        return;
    if (mode == ProjectionMode::Parallel) {
        orthoHeight = orthoHeightFromPerspectiveDistance();
    } else {
        distance = std::clamp(perspectiveDistanceFromOrthoHeight(), kMinDistance, kMaxDistance);
    }
    projectionMode = mode;
}
void CameraController::toggleProjectionMode() {
    setProjectionMode(projectionMode == ProjectionMode::Perspective ? ProjectionMode::Parallel : ProjectionMode::Perspective);
}

void CameraController::normalizeYaw() {
    while (yaw < 0.0f)
        yaw += 360.0f;
    while (yaw >= 360.0f)
        yaw -= 360.0f;
}

void CameraController::clampPitch() {
    if (pitch > 90.0f)
        pitch = 90.0f;
    if (pitch < -90.0f)
        pitch = -90.0f;
}

float CameraController::perspectiveDistanceFromOrthoHeight() const {
    float radians = fieldOfView * static_cast<float>(M_PI) / 180.0f;
    float denom = std::tan(radians * 0.5f);
    if (denom <= 1e-5f)
        denom = 1e-5f;
    return orthoHeight * 0.5f / denom;
}

float CameraController::orthoHeightFromPerspectiveDistance() const {
    float radians = fieldOfView * static_cast<float>(M_PI) / 180.0f;
    float scale = std::tan(radians * 0.5f);
    return std::clamp(distance * 2.0f * scale, kMinOrthoHeight, kMaxOrthoHeight);
}
