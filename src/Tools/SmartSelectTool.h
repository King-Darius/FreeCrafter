#pragma once

#include "Tool.h"

#include <vector>

class SmartSelectTool : public Tool {
public:
    SmartSelectTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "SmartSelectTool"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onKeyDown(int key) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    PreviewState buildPreview() const override;

private:
    GeometryObject* pickObjectAt(const Vector3& worldPoint);
    bool pointerToWorld(const PointerInput& input, Vector3& out) const;
    void applySelection(const std::vector<GeometryObject*>& hits, bool additive, bool toggle);
    void selectSingle(const PointerInput& input);
    void selectByRectangle(const PointerInput& input);
    void clearSelection();

    int anchorX = 0;
    int anchorY = 0;
    int currentX = 0;
    int currentY = 0;
    bool dragging = false;
    bool rectangleValid = false;
    Vector3 rectStart;
    Vector3 rectEnd;
    Vector3 anchorWorld;
    bool anchorWorldValid = false;
};

