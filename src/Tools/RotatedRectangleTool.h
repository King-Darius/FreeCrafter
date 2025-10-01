#pragma once

#include "Tool.h"

#include <vector>

class RotatedRectangleTool : public Tool {
public:
    RotatedRectangleTool(GeometryKernel* geometry, CameraController* camera);

    const char* getName() const override { return "RotatedRectangleTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }

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
    bool buildRectangle(const Vector3& candidate, std::vector<Vector3>& out) const;
    void reset();

    std::vector<Vector3> anchors;
    std::vector<Vector3> previewRectangle;
    Vector3 hoverPoint{ 0.0f, 0.0f, 0.0f };
    bool hoverValid = false;
    bool previewValid = false;
};

