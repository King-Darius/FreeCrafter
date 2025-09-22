#pragma once
#include "Tool.h"

class SelectionTool : public Tool {
public:
    SelectionTool(GeometryKernel* g, CameraController* c) : Tool(g,c) {}
    const char* getName() const override { return "SelectionTool"; }
    std::string getHint() const override { return "Select: Click to pick geometry. Hold Shift to toggle."; }
    void onMouseDown(int x, int y) override;
    void onMouseUp(int x, int y) override;
    void onKeyPress(char key) override;
private:
    GeometryObject* lastSelected = nullptr;
    GeometryObject* pickObjectAt(int sx, int sy);
};
