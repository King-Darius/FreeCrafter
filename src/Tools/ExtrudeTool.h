#pragma once
#include "Tool.h"
#include "../Scene/Document.h"

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

    enum class Stage {
        SelectingProfile,
        SelectingPathOrHeight,
        Dragging
    };

    bool computeExtrusionFrame();
    bool projectPointerToPlane(const PointerInput& input, Vector3& out) const;
    bool computePointerRay(const PointerInput& input, Vector3& origin, Vector3& direction) const;
    bool pickCurveAtPointer(const PointerInput& input, Curve*& outCurve,
        Scene::Document::ObjectId& outId, bool requireFace, bool requireNoFace) const;
    bool trySelectProfile(const PointerInput& input);
    bool trySelectPath(const PointerInput& input);
    void beginDrag(const PointerInput& input);
    void updateDragPreview(const PointerInput& input);
    void reset();
    float clampDistance(float value) const;

    Curve* profileCurve = nullptr;
    Curve* pathCurve = nullptr;
    Scene::Document::ObjectId profileId = 0;
    Scene::Document::ObjectId pathId = 0;
    Mode mode = Mode::None;
    Stage stage = Stage::SelectingProfile;
    Vector3 anchorPoint{ 0.0f, 0.0f, 0.0f };
    Vector3 baseDirection{ 0.0f, 1.0f, 0.0f };
    float previewDistance = 0.0f;
    float defaultDistance = 0.0f;
    bool pathClosed = false;
    bool dragging = false;
};
