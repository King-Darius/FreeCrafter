#include "CameraController.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CameraController::CameraController()
    : yaw(45.0f), pitch(-30.0f), distance(20.0f), targetX(0.0f), targetY(0.0f), targetZ(0.0f) {}

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
    if (yaw < 0) yaw += 360.0f;
    if (yaw >= 360.0f) yaw -= 360.0f;
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

    const float panSpeed = 0.0025f * distance;
    targetX -= (rightX * dx + upX * dy) * panSpeed;
    targetY -= (rightY * dx + upY * dy) * panSpeed;
    targetZ -= (rightZ * dx + upZ * dy) * panSpeed;
}

void CameraController::zoomCamera(float delta) {
    const float zoomSpeed = 1.1f;
    if (delta > 0) distance /= zoomSpeed;
    else distance *= zoomSpeed;
    if (distance < 1.0f) distance = 1.0f;
    if (distance > 2000.0f) distance = 2000.0f;
}

void CameraController::setTarget(float tx, float ty, float tz) { targetX = tx; targetY = ty; targetZ = tz; }
void CameraController::setDistance(float dist) { distance = dist; }
float CameraController::getYaw() const { return yaw; }
float CameraController::getPitch() const { return pitch; }
float CameraController::getDistance() const { return distance; }
void CameraController::getTarget(float& tx, float& ty, float& tz) const { tx = targetX; ty = targetY; tz = targetZ; }
