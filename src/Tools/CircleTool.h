#pragma once

#include "Tool.h"

#include <vector>

class CircleTool : public Tool {
public:
    CircleTool(GeometryKernel* geometry, CameraController* camera);

    const char* getName() const override { return "CircleTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }
    OverrideResult applyMeasurementOverride(double value) override;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCommit() override;
    void onCancel() override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    void updatePreview(const Vector3& candidate, bool valid);
    void buildCircle(const Vector3& center, float radius);
    void reset();

    Vector3 centerPoint{ 0.0f, 0.0f, 0.0f };
    bool hasCenter = false;
    Vector3 hoverPoint{ 0.0f, 0.0f, 0.0f };
    bool hoverValid = false;
    std::vector<Vector3> previewCircle;
    bool previewValid = false;
};

