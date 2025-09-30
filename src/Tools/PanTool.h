#pragma once

#include "Tool.h"

class PanTool : public PointerDragTool {
public:
    PanTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "PanTool"; }
    bool isNavigationTool() const override { return true; }

protected:
    void onDragUpdate(const PointerInput& input, float dx, float dy) override;
};
