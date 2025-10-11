#pragma once

#include "Tool.h"

#include "../GeometryKernel/Curve.h"
#include "../Phase6/AdvancedModeling.h"
#include "../Scene/Document.h"

#include <vector>

class ChamferTool : public Tool {
public:
    ChamferTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "ChamferTool"; }
    MeasurementKind getMeasurementKind() const override { return MeasurementKind::Distance; }
    OverrideResult applyMeasurementOverride(double value) override;

    void setRoundCornerOptions(const Phase6::RoundCornerOptions& opts);
    Phase6::RoundCornerOptions roundCornerOptions() const { return options; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCommit() override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    PreviewState buildPreview() const override;

private:
    bool rebuildPreview();
    void resetState();
    bool selectTarget();

    Phase6::RoundCornerOptions options;
    Curve* targetCurve = nullptr;
    Scene::Document::ObjectId targetId = 0;
    std::vector<Vector3> previewLoop;
    std::vector<bool> previewHardness;
    bool previewValid = false;
};
