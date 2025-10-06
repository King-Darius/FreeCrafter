#pragma once

#include "Tool.h"

#include <string>
#include <vector>

class OffsetTool : public Tool {
public:
    OffsetTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Offset"; }
    void setDistance(float d) { distance = d; }

protected:
    void onPointerDown(const PointerInput& input) override;

private:
    float distance = 1.0f;
};

class PushPullTool : public Tool {
public:
    PushPullTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "PushPull"; }
    void setDistance(float d) { distance = d; }

protected:
    void onPointerDown(const PointerInput& input) override;

private:
    float distance = 1.0f;
};

class FollowMeTool : public Tool {
public:
    FollowMeTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "FollowMe"; }
    void setProfile(GeometryObject* obj) { profile = obj; }
    void setPath(GeometryObject* obj) { path = obj; }

protected:
    void onPointerDown(const PointerInput& input) override;

private:
    GeometryObject* profile = nullptr;
    GeometryObject* path = nullptr;
};

class PaintBucketTool : public Tool {
public:
    PaintBucketTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "PaintBucket"; }
    void setMaterialName(std::string name) { materialName = std::move(name); }

protected:
    void onPointerDown(const PointerInput& input) override;

private:
    std::string materialName;
};

class TextTool : public Tool {
public:
    TextTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Text"; }
    void setText(std::string value) { text = std::move(value); }
    void setHeight(float h) { height = h; }

protected:
    void onPointerDown(const PointerInput& input) override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;

    std::string text;
    float height = 1.0f;
};

class DimensionTool : public Tool {
public:
    DimensionTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Dimension"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;
    void finalizeDimension(const Vector3& endPoint);

    Vector3 startPoint{};
    Vector3 previewPoint{};
    bool hasStart = false;
    bool previewValid = false;
};

class TapeMeasureTool : public Tool {
public:
    TapeMeasureTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "TapeMeasure"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;

    Vector3 startPoint{};
    Vector3 previewPoint{};
    bool hasStart = false;
    bool previewValid = false;
};

class ProtractorTool : public Tool {
public:
    ProtractorTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Protractor"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;
    void finalizeAngle();

    std::vector<Vector3> points;
    Vector3 previewPoint{};
    bool previewValid = false;
};

class AxesTool : public Tool {
public:
    AxesTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Axes"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;
    void finalizeAxes(const Vector3& yDirection);

    std::vector<Vector3> anchors;
    Vector3 previewPoint{};
    bool previewValid = false;
};

