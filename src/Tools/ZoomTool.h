#pragma once

#include "Tool.h"

class ZoomTool : public PointerDragTool {
public:
    ZoomTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "ZoomTool"; }
    bool isNavigationTool() const override { return true; }

    void setZoomToCursor(bool enabled) { zoomToCursor = enabled; }
    void setDragSensitivity(float value) { dragSensitivity = value; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onDragStart(const PointerInput& input) override;
    void onDragUpdate(const PointerInput& input, float dx, float dy) override;
    void onDragEnd(const PointerInput& input) override;
    void onDragCanceled() override;

private:
    bool zoomToCursor = true;
    float dragSensitivity = 0.08f;

    void applyZoomDelta(const PointerInput& input, float delta);
    bool zoomSelection();
    bool zoomExtents();
    bool zoomToBounds(bool selectedOnly);
};
