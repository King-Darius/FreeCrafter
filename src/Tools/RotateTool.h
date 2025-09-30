#pragma once

#include "Tool.h"

#include <vector>

class RotateTool : public Tool {
public:
    RotateTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "RotateTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Angle; }
    OverrideResult applyMeasurementOverride(double value) override;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onCommit() override;
    PreviewState buildPreview() const override;

private:
    bool pointerToWorld(const PointerInput& input, Vector3& out) const;
    std::vector<GeometryObject*> gatherSelection() const;
    Vector3 determineAxis() const;
    void applyRotation(float angleRadians);

    bool dragging = false;
    Vector3 pivot;
    Vector3 axis{ 0.0f, 1.0f, 0.0f };
    Vector3 startVector;
    float currentAngle = 0.0f;
    std::vector<GeometryObject*> selection;
};

