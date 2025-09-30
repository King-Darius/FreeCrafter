#include "OrbitTool.h"

#include <algorithm>
#include <cmath>

OrbitTool::OrbitTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void OrbitTool::onPointerDown(const PointerInput& input)
{
    dragging = true;
    lastX = input.x;
    lastY = input.y;
    pixelScale = std::max(input.devicePixelRatio, 1.0f);
    setState(State::Active);
}

void OrbitTool::onPointerMove(const PointerInput& input)
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

    if (getModifiers().shift) {
        camera->panCamera(dx, dy);
    } else {
        camera->rotateCamera(dx, dy);
    }
}

void OrbitTool::onPointerUp(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    dragging = false;
    setState(State::Idle);
}

void OrbitTool::onCancel()
{
    dragging = false;
    setState(State::Idle);
}
