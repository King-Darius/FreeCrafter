#include "ToolManager.h"

#include <algorithm>
#include <cstring>

ToolManager::ToolManager(GeometryKernel* g, CameraController* c)
{
    tools.push_back(std::make_unique<SelectionTool>(g,c));
    tools.push_back(std::make_unique<SketchTool>(g,c));
    tools.push_back(std::make_unique<ExtrudeTool>(g,c));
    propagateViewport();
    active = tools.front().get();
}

void ToolManager::activateTool(const char* n){
    for (auto& t : tools) {
        if (std::strcmp(t->getName(), n) == 0) { active = t.get(); return; }
    }
}

void ToolManager::setViewportSize(int w, int h)
{
    viewportWidth = std::max(1, w);
    viewportHeight = std::max(1, h);
    propagateViewport();
}

void ToolManager::propagateViewport()
{
    for (auto& tool : tools) {
        tool->setViewportSize(viewportWidth, viewportHeight);
    }
}
