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
private:
    std::vector<std::unique_ptr<Tool>> tools; Tool* active=nullptr;
};
