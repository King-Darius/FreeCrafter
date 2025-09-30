#pragma once

#include "Tool.h"

class PanTool : public PointerDragTool {
public:
    PanTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "PanTool"; }
    bool isNavigationTool() const override { return true; }

protected:
    void onPointerMove(const PointerInput& input) override;
};
