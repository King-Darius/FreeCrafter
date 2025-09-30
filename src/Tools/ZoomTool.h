#pragma once

#include "Tool.h"

class ZoomTool : public Tool {
public:
    ZoomTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "ZoomTool"; }
    bool isNavigationTool() const override { return true; }

    void setZoomToCursor(bool enabled) { zoomToCursor = enabled; }
    void setDragSensitivity(float value) { dragSensitivity = value; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCancel() override;

private:
    bool dragging = false;
    int lastX = 0;
    int lastY = 0;
    float pixelScale = 1.0f;
    bool zoomToCursor = true;
    float dragSensitivity = 0.08f;

    void applyZoomDelta(const PointerInput& input, float delta);
    bool zoomSelection();
    bool zoomExtents();
    bool zoomToBounds(bool selectedOnly);
};
