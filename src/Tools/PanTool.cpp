#include "PanTool.h"

#include <algorithm>
#include <cmath>

PanTool::PanTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void PanTool::onPointerDown(const PointerInput& input)
{
    dragging = true;
    lastX = input.x;
    lastY = input.y;
    pixelScale = std::max(input.devicePixelRatio, 1.0f);
    setState(State::Active);
}

void PanTool::onPointerMove(const PointerInput& input)
{
    if (!dragging || !camera)
        return;
    float scale = std::max(pixelScale, 1.0f);
    float dx = (input.x - lastX) / scale;
    float dy = (input.y - lastY) / scale;
    lastX = input.x;
    lastY = input.y;
    if (std::fabs(dx) < 1e-3f && std::fabs(dy) < 1e-3f)
        return;

    if (getModifiers().alt)
        camera->panCamera(dx * 0.5f, dy * 0.5f);
    else
        camera->panCamera(dx, dy);
}

void PanTool::onPointerUp(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    dragging = false;
    setState(State::Idle);
}

void PanTool::onCancel()
{
    dragging = false;
    setState(State::Idle);
}
