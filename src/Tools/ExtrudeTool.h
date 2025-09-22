#pragma once
#include "Tool.h"

class ExtrudeTool : public Tool {
public:
    ExtrudeTool(GeometryKernel* g, CameraController* c) : Tool(g,c) {}
    const char* getName() const override { return "ExtrudeTool"; }
    void onMouseDown(int x,int y) override;
    void onMouseMove(int,int) override {}
    void onMouseUp(int,int) override {}
    std::string getHint() const override { return "Extrude: Click a curve to pull it upward. Type a distance for precision."; }
    std::string getMeasurementPrompt() const override { return "Distance"; }
    bool acceptsNumericInput() const override { return true; }
    bool applyNumericInput(double value) override;

private:
    double pendingDistance = 1.0;
};
