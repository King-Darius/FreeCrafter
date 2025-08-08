#pragma once
#include "Tool.h"

class ExtrudeTool : public Tool {
public:
    ExtrudeTool(GeometryKernel* g, CameraController* c) : Tool(g,c) {}
    const char* getName() const override { return "ExtrudeTool"; }
    void onMouseDown(int x,int y) override;
    void onMouseMove(int,int) override {}
    void onMouseUp(int,int) override {}
};
