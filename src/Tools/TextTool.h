#pragma once

#include "Tool.h"

#include <string>

namespace Scene {
class Document;
}

class TextTool : public Tool {
public:
    TextTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* document);

    const char* getName() const override { return "TextTool"; }
    void setText(const std::string& text) { content = text; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;

    Scene::Document* document = nullptr;
    std::string content = "Label";
    Vector3 hoverPoint;
    bool hoverValid = false;
};
