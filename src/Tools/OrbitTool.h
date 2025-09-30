#pragma once

#include "Tool.h"

class OrbitTool : public PointerDragTool {
public:
    OrbitTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "OrbitTool"; }
    bool isNavigationTool() const override { return true; }

protected:
    void onDragUpdate(const PointerInput& input, float dx, float dy) override;
};
