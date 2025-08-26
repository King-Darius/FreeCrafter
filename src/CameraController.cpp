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
    // pan relative to camera
    float radYaw = yaw * (float)M_PI / 180.0f;
    float rightX = cos(radYaw), rightZ = -sin(radYaw);
    float upX = 0.0f, upZ = 0.0f; // ground plane
    const float panSpeed = 0.02f * distance;
    targetX -= (rightX * dx + upX * dy) * panSpeed;
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
