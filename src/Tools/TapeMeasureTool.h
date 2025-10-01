#pragma once

#include "Tool.h"

namespace Scene {
class Document;
}

class TapeMeasureTool : public Tool {
public:
    TapeMeasureTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* document);

    const char* getName() const override { return "TapeMeasureTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }

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
};
