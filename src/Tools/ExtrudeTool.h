#pragma once
#include "Tool.h"

class ExtrudeTool : public Tool {
public:
    ExtrudeTool(GeometryKernel* g, CameraController* c) : Tool(g,c) {}
    const char* getName() const override { return "ExtrudeTool"; }
protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput&) override {}
    void onPointerUp(const PointerInput&) override {}
};
