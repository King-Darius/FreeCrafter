#include "ToolManager.h"
#include <cstring>

ToolManager::ToolManager(GeometryKernel* g, CameraController* c){
    tools.push_back(std::make_unique<SelectionTool>(g,c));
    tools.push_back(std::make_unique<SketchTool>(g,c));
    tools.push_back(std::make_unique<ExtrudeTool>(g,c));
    active = tools.front().get();
}

void ToolManager::activateTool(const char* n){
    for (auto& t : tools) {
        if (std::strcmp(t->getName(), n) == 0) { active = t.get(); return; }
    }
}
