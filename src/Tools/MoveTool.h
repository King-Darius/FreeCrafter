#pragma once

#include "Tool.h"

#include <vector>

class MoveTool : public Tool {
public:
    MoveTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "MoveTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }
    OverrideResult applyMeasurementOverride(double value) override;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onCommit() override;
    PreviewState buildPreview() const override;

private:
    bool pointerToWorld(const PointerInput& input, Vector3& out) const;
    Vector3 applyAxisConstraint(const Vector3& delta) const;
    std::vector<GeometryObject*> gatherSelection() const;
    void applyTranslation(const Vector3& delta);

    bool dragging = false;
    Vector3 anchor;
    Vector3 translation;
    Vector3 pivot;
    std::vector<GeometryObject*> selection;
};

