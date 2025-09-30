#pragma once

#include "Tool.h"
#include "../Scene/Document.h"

class SectionTool : public Tool {
public:
    SectionTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* document);

    const char* getName() const override { return "SectionTool"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onCancel() override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    void updatePreviewFromInference(const Interaction::InferenceResult& result);
    void refreshPreviewValidity();

    Scene::Document* document = nullptr;
    bool pendingPlacement = false;
    Vector3 previewOrigin{ 0.0f, 0.0f, 0.0f };
    Vector3 previewNormal{ 0.0f, 1.0f, 0.0f };
    bool previewValid = false;
};
