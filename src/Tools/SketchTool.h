#pragma once
#include "Tool.h"
#include <vector>

class SketchTool : public Tool {
public:
    SketchTool(GeometryKernel* g, CameraController* c) : Tool(g,c) {}
    const char* getName() const override { return "SketchTool"; }
    void onMouseDown(int x,int y) override;
    void onMouseMove(int x,int y) override;
    void onMouseUp(int x,int y) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
private:
    bool resolvePoint(int x, int y, Vector3& out) const;
    bool resolveFallback(int x, int y, Vector3& out) const;

    bool drawing=false;
    std::vector<Vector3> pts;
    Vector3 previewPoint;
    bool previewValid = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
};
