#include "OrbitTool.h"

#include <cmath>

OrbitTool::OrbitTool(GeometryKernel* g, CameraController* c)
    : PointerDragTool(g, c)
{
}

void OrbitTool::onDragUpdate(const PointerInput& input, float dx, float dy)
{
    if (!camera)
        return;
    if (std::fabs(dx) < 1e-3f && std::fabs(dy) < 1e-3f)
        return;

    if (getModifiers().shift) {
        camera->panCamera(dx, dy);
    } else {
        camera->rotateCamera(dx, dy);
    }
}
