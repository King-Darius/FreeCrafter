#pragma once

#include "../GeometryKernel/Vector3.h"
#include "../CameraController.h"

namespace ToolHelpers {

bool screenToGround(CameraController* cam, int x, int y, int viewportW, int viewportH, Vector3& out);
void axisSnap(Vector3& point);

}

