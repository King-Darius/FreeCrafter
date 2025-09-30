#include "OrbitTool.h"

#include <cmath>

OrbitTool::OrbitTool(GeometryKernel* g, CameraController* c)
    : PointerDragTool(g, c)
{
}

void OrbitTool::onPointerMove(const PointerInput& input)
{
    if (!hasActiveDrag() || !camera)
        return;
    float dx = 0.0f;
    float dy = 0.0f;
    if (!updateDragDelta(input, dx, dy))
        return;
    if (std::fabs(dx) < 1e-3f && std::fabs(dy) < 1e-3f)
        return;

    if (getModifiers().shift) {
        camera->panCamera(dx, dy);
    } else {
        camera->rotateCamera(dx, dy);
    }
}
