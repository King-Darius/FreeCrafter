#pragma once
#include <vector>
#include <memory>
#include "Tool.h"
#include "SelectionTool.h"
#include "SketchTool.h"
#include "ExtrudeTool.h"

class ToolManager {
public:
    ToolManager(GeometryKernel* g, CameraController* c);
    Tool* getActiveTool() const { return active; }
    void activateTool(const char* name);
    void setViewportSize(int w, int h);
private:
    void propagateViewport();

    std::vector<std::unique_ptr<Tool>> tools;
    Tool* active=nullptr;
    int viewportWidth = 1;
    int viewportHeight = 1;
};
