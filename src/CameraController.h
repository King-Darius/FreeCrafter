#pragma once
#include <cmath>

class CameraController {
public:
    CameraController();
    void getCameraPosition(float& x, float& y, float& z) const;
    void rotateCamera(float dx, float dy);
    void panCamera(float dx, float dy);
    void zoomCamera(float delta);
    void setTarget(float tx, float ty, float tz);
    void setDistance(float dist);
    float getYaw() const;
    float getPitch() const;
    float getDistance() const;
    void getTarget(float& tx, float& ty, float& tz) const;

private:
    float yaw;
    float pitch;
    float distance;
    float targetX, targetY, targetZ;
};
