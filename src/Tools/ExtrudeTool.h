#pragma once
#include "Tool.h"

class Curve;

class ExtrudeTool : public Tool {
public:
    ExtrudeTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "ExtrudeTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }
    OverrideResult applyMeasurementOverride(double value) override;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCancel() override;
    void onCommit() override;
    void onStateChanged(State previous, State next) override;
    PreviewState buildPreview() const override;

private:
    enum class Mode {
        None,
        Linear,
        Path
    };

    bool prepareExtrusionContext();
    bool projectPointerToPlane(const PointerInput& input, Vector3& out) const;
    void reset();
    float clampDistance(float value) const;

    Curve* profileCurve = nullptr;
    Curve* pathCurve = nullptr;
    Scene::Document::ObjectId profileId = 0;
    Scene::Document::ObjectId pathId = 0;
    Mode mode = Mode::None;
    Vector3 anchorPoint{ 0.0f, 0.0f, 0.0f };
    Vector3 baseDirection{ 0.0f, 1.0f, 0.0f };
    float previewDistance = 0.0f;
    float defaultDistance = 0.0f;
    bool dragging = false;
};
