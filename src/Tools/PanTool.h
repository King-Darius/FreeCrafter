#pragma once

#include "Tool.h"

class PanTool : public Tool {
public:
    PanTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "PanTool"; }
    bool isNavigationTool() const override { return true; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCancel() override;

private:
    bool dragging = false;
    int lastX = 0;
    int lastY = 0;
    float pixelScale = 1.0f;
};
