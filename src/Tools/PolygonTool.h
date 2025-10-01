#pragma once

#include "Tool.h"

#include <vector>

class PolygonTool : public Tool {
public:
    explicit PolygonTool(GeometryKernel* geometry, CameraController* camera, int sides = 6);

    const char* getName() const override { return "PolygonTool"; }
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
    void buildPolygon(const Vector3& center, const Vector3& direction, float radius);
    void reset();

    int sides = 6;
    Vector3 centerPoint{ 0.0f, 0.0f, 0.0f };
    bool hasCenter = false;
    Vector3 previewDirection{ 1.0f, 0.0f, 0.0f };
    Vector3 hoverPoint{ 0.0f, 0.0f, 0.0f };
    bool hoverValid = false;
    std::vector<Vector3> previewPolygon;
    bool previewValid = false;
};

