#pragma once

#include "Tool.h"

#include <optional>

namespace Scene {
class Document;
}

class DimensionTool : public Tool {
public:
    DimensionTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* document);

    const char* getName() const override { return "DimensionTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }
    OverrideResult applyMeasurementOverride(double value) override;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    void reset();

    Scene::Document* document = nullptr;
    Vector3 firstPoint;
    bool hasFirstPoint = false;
    Vector3 hoverPoint;
    bool hoverValid = false;
    std::optional<float> overrideDistance;
};
