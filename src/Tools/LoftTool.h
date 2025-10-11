#pragma once

#include "Tool.h"

#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Solid.h"
#include "../Phase6/AdvancedModeling.h"
#include "../Scene/Document.h"

#include <vector>

class LoftTool : public Tool {
public:
    LoftTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "LoftTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Count; }
    OverrideResult applyMeasurementOverride(double value) override;

    void setLoftOptions(const Phase6::LoftOptions& opts);
    Phase6::LoftOptions loftOptions() const { return options; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCommit() override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    PreviewState buildPreview() const override;

private:
    bool collectSelection();
    bool rebuildPreview();
    void resetState();

    Phase6::LoftOptions options;
    Scene::Document::ObjectId startId = 0;
    Scene::Document::ObjectId endId = 0;
    Curve* startCurve = nullptr;
    Curve* endCurve = nullptr;
    GeometryKernel previewKernel;
    Solid* previewSolid = nullptr;
    bool previewValid = false;
};
