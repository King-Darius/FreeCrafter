#pragma once

#include "Tool.h"

class PaintBucketTool : public Tool {
public:
    PaintBucketTool(GeometryKernel* geometry, CameraController* camera);

    const char* getName() const override { return "PaintBucketTool"; }
    void setPaintColor(const Vector3& color) { paintColor = color; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onCancel() override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    GeometryObject* findObjectAtPoint(const Vector3& point) const;
    void updateHover(const Vector3& point);
    void clearHover();

    Vector3 paintColor{ 0.9f, 0.2f, 0.2f };
    GeometryObject* hoverObject = nullptr;
    Vector3 hoverPoint;
    bool hoverValid = false;
};
