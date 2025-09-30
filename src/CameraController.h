#pragma once
#include <cmath>

class CameraController {
public:
    enum class ProjectionMode {
        Perspective,
        Parallel
    };

    CameraController();
    void getCameraPosition(float& x, float& y, float& z) const;
    void rotateCamera(float dx, float dy);
    void panCamera(float dx, float dy);
    void zoomCamera(float delta);
    void setTarget(float tx, float ty, float tz);
    void setDistance(float dist);
    void setYaw(float newYaw);
    void setPitch(float newPitch);
    void setYawPitch(float newYaw, float newPitch);
    float getYaw() const;
    float getPitch() const;
    float getDistance() const;
    void getTarget(float& tx, float& ty, float& tz) const;
    float getFieldOfView() const;
    void setFieldOfView(float degrees);
    float getOrthoHeight() const;
    void setOrthoHeight(float height);
    ProjectionMode getProjectionMode() const;
    void setProjectionMode(ProjectionMode mode);
    void toggleProjectionMode();

private:
    void normalizeYaw();
    void clampPitch();
    float perspectiveDistanceFromOrthoHeight() const;
    float orthoHeightFromPerspectiveDistance() const;

    float yaw;
    float pitch;
    float distance;
    float targetX, targetY, targetZ;
    ProjectionMode projectionMode;
    float fieldOfView;
    float orthoHeight;
};
