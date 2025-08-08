#include "CameraController.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CameraController::CameraController()
    : yaw(45.0f), pitch(-30.0f), distance(10.0f), targetX(0.0f), targetY(0.0f), targetZ(0.0f) {}

void CameraController::getCameraPosition(float& x, float& y, float& z) const {
    float radYaw = yaw * (float)M_PI / 180.0f;
    float radPitch = pitch * (float)M_PI / 180.0f;
    float cx = distance * sin(radYaw) * cos(radPitch);
    float cy = distance * sin(radPitch);
    float cz = distance * cos(radYaw) * cos(radPitch);
    x = targetX + cx; y = targetY + cy; z = targetZ + cz;
}

void CameraController::rotateCamera(float dx, float dy) {
    const float rotateSpeed = 0.2f;
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
    float fx = -sinf(radYaw) * cosf(radPitch);
    float fy = -sinf(radPitch);
    float fz = -cosf(radYaw) * cosf(radPitch);
    float rx = fz, ry = 0.0f, rz = -fx;
    float rLen = sqrtf(rx*rx + ry*ry + rz*rz);
    if (rLen > 0) { rx /= rLen; ry /= rLen; rz /= rLen; }
    float ux = ry * fz - rz * fy;
    float uy = rz * fx - rx * fz;
    float uz = rx * fy - ry * fx;
    float uLen = sqrtf(ux*ux + uy*uy + uz*uz);
    if (uLen > 0) { ux /= uLen; uy /= uLen; uz /= uLen; }
    const float panSpeed = 0.002f * distance;
    targetX += - (rx * dx * panSpeed) + - (ux * dy * panSpeed);
    targetY += - (ry * dx * panSpeed) + - (uy * dy * panSpeed);
    targetZ += - (rz * dx * panSpeed) + - (uz * dy * panSpeed);
}

void CameraController::zoomCamera(float delta) {
    const float zoomSpeed = 0.1f;
    distance -= delta * zoomSpeed;
    if (distance < 0.5f) distance = 0.5f;
    if (distance > 100.0f) distance = 100.0f;
}

void CameraController::setTarget(float tx, float ty, float tz) { targetX = tx; targetY = ty; targetZ = tz; }
void CameraController::setDistance(float dist) { if (dist > 0.1f) distance = dist; }
float CameraController::getYaw() const { return yaw; }
float CameraController::getPitch() const { return pitch; }
float CameraController::getDistance() const { return distance; }
void CameraController::getTarget(float& tx, float& ty, float& tz) const { tx = targetX; ty = targetY; tz = targetZ; }
