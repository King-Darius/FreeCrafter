#pragma once

#include "GeometryKernel/Vector3.h"

class CameraController;

namespace CameraNavigation {

bool zoomAboutCursor(CameraController& camera, float delta, int pixelX, int pixelY, int viewportWidth, int viewportHeight,
                     bool zoomToCursor);

bool frameBounds(CameraController& camera, const Vector3& minBounds, const Vector3& maxBounds, int viewportWidth,
                 int viewportHeight);

bool computeRay(const CameraController& camera, int pixelX, int pixelY, int viewportWidth, int viewportHeight,
                Vector3& origin, Vector3& direction);

}
