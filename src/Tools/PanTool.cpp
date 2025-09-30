#include "PanTool.h"

#include <cmath>

PanTool::PanTool(GeometryKernel* g, CameraController* c)
    : PointerDragTool(g, c)
{
}

void PanTool::onDragUpdate(const PointerInput& input, float dx, float dy)
{
    if (!camera)
        return;
    if (std::fabs(dx) < 1e-3f && std::fabs(dy) < 1e-3f)
        return;

    if (getModifiers().alt)
        camera->panCamera(dx * 0.5f, dy * 0.5f);
    else
        camera->panCamera(dx, dy);
}
