#pragma once

#include "Tool.h"

namespace Scene {
class Document;
}

class AxesTool : public Tool {
public:
    AxesTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* document);

    const char* getName() const override { return "AxesTool"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    void reset();

    Scene::Document* document = nullptr;
    Vector3 hoverPoint;
    bool hoverValid = false;
};
