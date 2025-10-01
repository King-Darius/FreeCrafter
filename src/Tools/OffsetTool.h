#pragma once

#include "Tool.h"

#include <vector>

class OffsetTool : public Tool {
public:
    OffsetTool(GeometryKernel* geometry, CameraController* camera);

    const char* getName() const override { return "OffsetTool"; }
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
    Curve* pickSelectedCurve() const;
    Curve* pickCurveAtPoint(const Vector3& point) const;
    void prepareSource(Curve* curve);
    void updatePreview(float offsetValue);
    void updatePreviewFromPoint(const Vector3& point);
    void reset();
    float computeAverageRadius(const std::vector<Vector3>& loop) const;

    Curve* sourceCurve = nullptr;
    std::vector<Vector3> baseLoop;
    std::vector<Vector3> previewLoop;
    Vector3 centroid;
    float baseRadius = 0.0f;
    float pendingOffset = 1.0f;
    bool previewValid = false;
    bool hasFirstPoint = false;
    Vector3 hoverPoint;
    bool hoverValid = false;
};
