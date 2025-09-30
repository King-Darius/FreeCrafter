#pragma once

#include "Tool.h"

#include <vector>

class ScaleTool : public Tool {
public:
    ScaleTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "ScaleTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Scale; }
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
    void applyScale(const Vector3& factors);
    Vector3 determineAxis() const;

    bool dragging = false;
    bool axisScaling = false;
    Vector3 pivot;
    Vector3 axis{ 0.0f, 1.0f, 0.0f };
    Vector3 startVector;
    Vector3 scaleFactors{ 1.0f, 1.0f, 1.0f };
    std::vector<GeometryObject*> selection;
};

