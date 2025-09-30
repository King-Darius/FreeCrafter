#pragma once
#include "Tool.h"

class SelectionTool : public Tool {
public:
    SelectionTool(GeometryKernel* g, CameraController* c) : Tool(g,c) {}
    const char* getName() const override { return "SelectionTool"; }
    void onMouseDown(int x, int y) override;
    void onMouseUp(int x, int y) override;
    void onKeyPress(int key) override;
private:
    GeometryObject* lastSelected = nullptr;
    GeometryObject* pickObjectAt(const Vector3& worldPoint);
};
