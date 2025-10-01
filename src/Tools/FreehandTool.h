#pragma once

#include "Tool.h"

#include <vector>

class FreehandTool : public PointerDragTool {
public:
    FreehandTool(GeometryKernel* geometry, CameraController* camera);

    const char* getName() const override { return "FreehandTool"; }

protected:
    void onDragStart(const PointerInput& input) override;
    void onDragUpdate(const PointerInput& input, float dx, float dy) override;
    void onDragEnd(const PointerInput& input) override;
    void onDragCanceled() override;
    void onPointerHover(const PointerInput& input) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool samplePoint(const PointerInput& input, Vector3& out) const;
    void addPoint(const Vector3& point);
    void resetStroke();

    std::vector<Vector3> stroke;
    std::vector<Vector3> previewStroke;
    Vector3 hoverPoint{ 0.0f, 0.0f, 0.0f };
    bool hoverValid = false;
};

