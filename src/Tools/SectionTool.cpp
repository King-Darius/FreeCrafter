#include "SectionTool.h"

#include "../Scene/SectionPlane.h"

#include <cmath>

namespace {
constexpr float kPlanePreviewExtent = 2.5f;
constexpr float kEpsilon = 1e-5f;

Vector3 safeNormalize(const Vector3& value, const Vector3& fallback)
{
    float len = value.length();
    if (len <= kEpsilon) {
        return fallback;
    }
    return value / len;
}
}

SectionTool::SectionTool(GeometryKernel* geometry, CameraController* camera, Scene::Document* doc)
    : Tool(geometry, camera)
    , document(doc)
{
}

void SectionTool::onPointerDown(const PointerInput&)
{
    if (!previewValid) {
        return;
    }
    pendingPlacement = true;
    setState(State::Active);
}

void SectionTool::onPointerUp(const PointerInput&)
{
    if (!pendingPlacement) {
        return;
    }
    pendingPlacement = false;
    setState(State::Idle);

    if (!previewValid || !document) {
        return;
    }

    if (document) {
        document->settings().setSectionPlanesVisible(true);
        document->settings().setSectionFillsVisible(true);
    }

    Scene::SectionPlane plane;
    plane.setFromOriginAndNormal(previewOrigin, previewNormal);
    plane.setActive(true);
    plane.setVisible(true);
    Scene::SectionFillStyle& style = plane.fillStyle();
    style.extent = kPlanePreviewExtent;
    style.fillEnabled = true;
    document->addSectionPlane(plane);
}

void SectionTool::onPointerHover(const PointerInput&)
{
    refreshPreviewValidity();
}

void SectionTool::onPointerMove(const PointerInput&)
{
    refreshPreviewValidity();
}

void SectionTool::onCancel()
{
    pendingPlacement = false;
    setState(State::Idle);
}

void SectionTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    updatePreviewFromInference(result);
}

Tool::PreviewState SectionTool::buildPreview() const
{
    PreviewState state;
    if (!previewValid) {
        return state;
    }

    Vector3 normal = safeNormalize(previewNormal, Vector3(0.0f, 1.0f, 0.0f));
    Vector3 fallback = std::fabs(normal.y) > 0.95f ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
    Vector3 xAxis = normal.cross(fallback);
    if (xAxis.lengthSquared() <= kEpsilon) {
        xAxis = normal.cross(Vector3(0.0f, 0.0f, 1.0f));
    }
    xAxis = safeNormalize(xAxis, Vector3(1.0f, 0.0f, 0.0f));
    Vector3 yAxis = xAxis.cross(normal);
    yAxis = safeNormalize(yAxis, Vector3(0.0f, 0.0f, 1.0f));

    float extent = kPlanePreviewExtent;
    Vector3 right = xAxis * extent;
    Vector3 up = yAxis * extent;

    PreviewPolyline poly;
    poly.closed = true;
    poly.points.push_back(previewOrigin + right + up);
    poly.points.push_back(previewOrigin - right + up);
    poly.points.push_back(previewOrigin - right - up);
    poly.points.push_back(previewOrigin + right - up);
    state.polylines.push_back(poly);
    return state;
}

void SectionTool::updatePreviewFromInference(const Interaction::InferenceResult& result)
{
    if (!result.isValid()) {
        previewValid = false;
        return;
    }
    previewOrigin = result.position;
    if (result.direction.lengthSquared() > kEpsilon) {
        previewNormal = result.direction.normalized();
    } else {
        previewNormal = Vector3(0.0f, 1.0f, 0.0f);
    }
    previewValid = true;
}

void SectionTool::refreshPreviewValidity()
{
    if (!previewValid) {
        return;
    }
    if (previewNormal.lengthSquared() <= kEpsilon) {
        previewValid = false;
    }
}
