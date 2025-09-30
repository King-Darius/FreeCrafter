#pragma once

#include "Tool.h"

#include <vector>

class LineTool : public Tool {
public:
    LineTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "LineTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }
    OverrideResult applyMeasurementOverride(double value) override;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCommit() override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;
    void resetChain();

    std::vector<Vector3> points;
    Vector3 previewPoint{ 0.0f, 0.0f, 0.0f };
    bool previewValid = false;
    int lastX = 0;
    int lastY = 0;
};

