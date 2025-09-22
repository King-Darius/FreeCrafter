#pragma once
#include "Tool.h"
#include <vector>

class SketchTool : public Tool {
public:
    SketchTool(GeometryKernel* g, CameraController* c) : Tool(g,c) {}
    const char* getName() const override { return "SketchTool"; }
    std::string getHint() const override { return "Sketch: Click to place points on ground plane. Enter closes segment."; }
    std::string getMeasurementPrompt() const override { return "Length"; }
    void onMouseDown(int x,int y) override;
    void onMouseMove(int x,int y) override;
    void onMouseUp(int x,int y) override;
private:
    bool drawing=false; std::vector<Vector3> pts;
};
