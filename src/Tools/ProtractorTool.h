#pragma once

#include "Tool.h"

#include <optional>

namespace Scene {
class Document;
}

class ProtractorTool : public Tool {
public:
    ProtractorTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* document);

    const char* getName() const override { return "ProtractorTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Angle; }
    OverrideResult applyMeasurementOverride(double value) override;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    void reset();
    float computeAngle(const Vector3& a, const Vector3& v, const Vector3& b) const;

    Scene::Document* document = nullptr;
    Vector3 firstPoint;
    Vector3 vertexPoint;
    bool hasFirstPoint = false;
    bool hasVertexPoint = false;
    Vector3 hoverPoint;
    bool hoverValid = false;
    std::optional<float> overrideAngle;
};
