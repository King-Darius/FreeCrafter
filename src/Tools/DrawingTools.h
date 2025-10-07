#pragma once

#include "Tool.h"

#include <vector>

class ArcTool : public Tool {
public:
    ArcTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Arc"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerHover(const PointerInput& input) override;
    void onCommit() override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    void onInferenceResultChanged(const Interaction::InferenceResult& result) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;
    void finalizeArc(const Vector3& bulgePoint);

    std::vector<Vector3> anchors;
    Vector3 previewPoint{};
    bool previewValid = false;
    int lastX = 0;
    int lastY = 0;
};

class CircleTool : public Tool {
public:
    CircleTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Circle"; }
    void setSegments(int count) { segments = std::max(8, count); }

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
    void finalizeCircle(const Vector3& radiusPoint);

    Vector3 center{};
    Vector3 previewPoint{};
    bool hasCenter = false;
    bool previewValid = false;
    int segments = 32;
    int lastX = 0;
    int lastY = 0;
};

class RectangleTool : public Tool {
public:
    RectangleTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Rectangle"; }

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
    void finalizeRectangle(const Vector3& oppositeCorner);

    Vector3 firstCorner{};
    Vector3 previewPoint{};
    bool hasFirstCorner = false;
    bool previewValid = false;
    int lastX = 0;
    int lastY = 0;
};

class PolygonTool : public Tool {
public:
    PolygonTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Polygon"; }
    void setSides(int count) { sides = std::max(3, count); }

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
    void finalizePolygon(const Vector3& radiusPoint);

    Vector3 center{};
    Vector3 previewPoint{};
    bool hasCenter = false;
    bool previewValid = false;
    int sides = 6;
    int lastX = 0;
    int lastY = 0;
};

class RotatedRectangleTool : public Tool {
public:
    RotatedRectangleTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "RotatedRectangle"; }

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
    void finalizeRectangle(const Vector3& heightPoint);

    std::vector<Vector3> corners;
    Vector3 previewPoint{};
    bool previewValid = false;
    int lastX = 0;
    int lastY = 0;
};

class FreehandTool : public Tool {
public:
    FreehandTool(GeometryKernel* g, CameraController* c);

    const char* getName() const override { return "Freehand"; }

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCancel() override;
    void onStateChanged(State previous, State next) override;
    PreviewState buildPreview() const override;

private:
    bool resolvePoint(const PointerInput& input, Vector3& out) const;
    bool resolveFallback(const PointerInput& input, Vector3& out) const;

    std::vector<Vector3> stroke;
    bool drawing = false;
};

