#include "ModificationTools.h"

#include "GroundProjection.h"
#include "../GeometryKernel/Curve.h"

#include <cmath>

namespace {

using ToolHelpers::axisSnap;
using ToolHelpers::screenToGround;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float ix, float iy)
        : x(ix)
        , y(iy)
    {
    }

    Vec2(const Vector3& v)
        : x(v.x)
        , y(v.z)
    {
    }

    Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
    Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    float dot(const Vec2& other) const { return x * other.x + y * other.y; }
    float cross(const Vec2& other) const { return x * other.y - y * other.x; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const
    {
        float len = length();
        if (len <= 1e-6f)
            return {};
        return { x / len, y / len };
    }
};

Vector3 toVec3(const Vec2& v)
{
    return { v.x, 0.0f, v.y };
}

std::vector<Vector3> offsetLoop(const std::vector<Vector3>& loop, float distance)
{
    size_t n = loop.size();
    if (n < 3)
        return {};
    std::vector<Vector3> result;
    result.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        Vec2 prev(loop[(i + n - 1) % n]);
        Vec2 curr(loop[i]);
        Vec2 next(loop[(i + 1) % n]);
        Vec2 dir1 = (curr - prev).normalized();
        Vec2 dir2 = (next - curr).normalized();
        Vec2 normal1(-dir1.y, dir1.x);
        Vec2 normal2(-dir2.y, dir2.x);
        Vec2 bisector = (normal1 + normal2).normalized();
        if (bisector.length() <= 1e-5f) {
            bisector = normal1;
        }
        float denom = bisector.dot(normal1);
        if (std::fabs(denom) < 1e-5f) {
            denom = denom < 0.0f ? -1.0f : 1.0f;
        }
        Vec2 offset = bisector * (distance / denom);
        result.push_back(toVec3(Vec2(curr) + offset));
    }
    return result;
}

GeometryObject* findSelectedCurve(GeometryKernel* geometry)
{
    if (!geometry)
        return nullptr;
    for (const auto& obj : geometry->getObjects()) {
        if (obj->getType() == ObjectType::Curve && obj->isSelected()) {
            return obj.get();
        }
    }
    return nullptr;
}

std::vector<GeometryObject*> findSelectedCurves(GeometryKernel* geometry)
{
    std::vector<GeometryObject*> curves;
    if (!geometry)
        return curves;
    for (const auto& obj : geometry->getObjects()) {
        if (obj->getType() == ObjectType::Curve && obj->isSelected()) {
            curves.push_back(obj.get());
        }
    }
    return curves;
}

GeometryObject* findPathCurve(GeometryKernel* geometry, GeometryObject* exclude)
{
    if (!geometry)
        return nullptr;
    const auto& objects = geometry->getObjects();
    for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
        GeometryObject* obj = it->get();
        if (obj == exclude)
            continue;
        if (obj->getType() == ObjectType::Curve)
            return obj;
    }
    return nullptr;
}

}

OffsetTool::OffsetTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void OffsetTool::onPointerDown(const PointerInput&)
{
    if (!geometry)
        return;
    auto curves = findSelectedCurves(geometry);
    for (GeometryObject* obj : curves) {
        auto* curve = static_cast<Curve*>(obj);
        auto loop = curve->getBoundaryLoop();
        auto offset = offsetLoop(loop, distance);
        if (!offset.empty()) {
            geometry->addCurve(offset);
        }
    }
}

PushPullTool::PushPullTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void PushPullTool::onPointerDown(const PointerInput&)
{
    if (!geometry)
        return;
    auto curves = findSelectedCurves(geometry);
    for (GeometryObject* obj : curves) {
        geometry->extrudeCurve(obj, distance);
    }
}

FollowMeTool::FollowMeTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void FollowMeTool::onPointerDown(const PointerInput&)
{
    if (!geometry)
        return;
    GeometryObject* profileCurve = profile ? profile : findSelectedCurve(geometry);
    if (!profileCurve || profileCurve->getType() != ObjectType::Curve)
        return;
    GeometryObject* pathCurve = path;
    if (!pathCurve)
        pathCurve = findPathCurve(geometry, profileCurve);
    if (!pathCurve || pathCurve->getType() != ObjectType::Curve)
        return;

    const auto& profileLoop = static_cast<Curve*>(profileCurve)->getBoundaryLoop();
    const auto& pathLoop = static_cast<Curve*>(pathCurve)->getBoundaryLoop();
    if (profileLoop.empty() || pathLoop.size() < 2)
        return;

    Vector3 start = pathLoop.front();
    for (const auto& point : pathLoop) {
        Vector3 offset = point - start;
        std::vector<Vector3> section;
        section.reserve(profileLoop.size());
        for (const auto& p : profileLoop) {
            section.push_back(p + Vector3(offset.x, 0.0f, offset.z));
        }
        geometry->addCurve(section);
    }
}

PaintBucketTool::PaintBucketTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void PaintBucketTool::onPointerDown(const PointerInput&)
{
    if (!geometry || materialName.empty())
        return;
    auto curves = findSelectedCurves(geometry);
    for (GeometryObject* obj : curves) {
        geometry->assignMaterial(obj, materialName);
    }
}

TextTool::TextTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void TextTool::onPointerDown(const PointerInput& input)
{
    if (!geometry || text.empty())
        return;
    Vector3 point;
    if (!resolvePoint(input, point))
        return;
    geometry->addTextAnnotation(point, text, height);
}

bool TextTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool TextTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out))
        return false;
    axisSnap(out);
    out.y = 0.0f;
    return true;
}

DimensionTool::DimensionTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void DimensionTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;
    if (!hasStart) {
        startPoint = point;
        hasStart = true;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
    } else {
        finalizeDimension(point);
        setState(State::Idle);
    }
}

void DimensionTool::onPointerMove(const PointerInput& input)
{
    if (!hasStart)
        return;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void DimensionTool::onPointerHover(const PointerInput& input)
{
    if (hasStart) {
        onPointerMove(input);
        return;
    }
    Vector3 point;
    if (resolvePoint(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else if (resolveFallback(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else {
        previewValid = false;
    }
}

void DimensionTool::onCancel()
{
    hasStart = false;
    previewValid = false;
}

void DimensionTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        hasStart = false;
        previewValid = false;
    }
}

void DimensionTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState DimensionTool::buildPreview() const
{
    PreviewState state;
    if (hasStart) {
        PreviewPolyline polyline;
        polyline.points = { startPoint };
        if (previewValid)
            polyline.points.push_back(previewPoint);
        state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool DimensionTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool DimensionTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out))
        return false;
    axisSnap(out);
    out.y = 0.0f;
    return true;
}

void DimensionTool::finalizeDimension(const Vector3& endPoint)
{
    if (!geometry)
        return;
    geometry->addDimension(startPoint, endPoint);
    hasStart = false;
    previewValid = false;
}

TapeMeasureTool::TapeMeasureTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void TapeMeasureTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;
    if (!hasStart) {
        startPoint = point;
        hasStart = true;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
    } else {
        if (geometry)
            geometry->addGuideLine(startPoint, point);
        hasStart = false;
        previewValid = false;
        setState(State::Idle);
    }
}

void TapeMeasureTool::onPointerMove(const PointerInput& input)
{
    if (!hasStart)
        return;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void TapeMeasureTool::onPointerHover(const PointerInput& input)
{
    if (hasStart) {
        onPointerMove(input);
        return;
    }
    Vector3 point;
    if (resolvePoint(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else if (resolveFallback(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else {
        previewValid = false;
    }
}

void TapeMeasureTool::onCancel()
{
    hasStart = false;
    previewValid = false;
}

void TapeMeasureTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        hasStart = false;
        previewValid = false;
    }
}

void TapeMeasureTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState TapeMeasureTool::buildPreview() const
{
    PreviewState state;
    if (hasStart) {
        PreviewPolyline polyline;
        polyline.points = { startPoint };
        if (previewValid)
            polyline.points.push_back(previewPoint);
        state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool TapeMeasureTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool TapeMeasureTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out))
        return false;
    axisSnap(out);
    out.y = 0.0f;
    return true;
}

ProtractorTool::ProtractorTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void ProtractorTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;
    points.push_back(point);
    if (points.size() == 1) {
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
    } else if (points.size() == 3) {
        finalizeAngle();
        setState(State::Idle);
    } else {
        previewPoint = point;
        previewValid = true;
    }
}

void ProtractorTool::onPointerMove(const PointerInput& input)
{
    if (points.empty())
        return;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void ProtractorTool::onPointerHover(const PointerInput& input)
{
    if (!points.empty()) {
        onPointerMove(input);
        return;
    }
    Vector3 point;
    if (resolvePoint(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else if (resolveFallback(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else {
        previewValid = false;
    }
}

void ProtractorTool::onCancel()
{
    points.clear();
    previewValid = false;
}

void ProtractorTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        points.clear();
        previewValid = false;
    }
}

void ProtractorTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!points.empty() && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    } else if (points.empty() && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState ProtractorTool::buildPreview() const
{
    PreviewState state;
    if (!points.empty()) {
        PreviewPolyline polyline;
        polyline.points = points;
        if (previewValid)
            polyline.points.push_back(previewPoint);
        state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool ProtractorTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool ProtractorTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out))
        return false;
    axisSnap(out);
    out.y = 0.0f;
    return true;
}

void ProtractorTool::finalizeAngle()
{
    if (!geometry || points.size() != 3)
        return;
    Vector3 vertex = points[1];
    Vector3 start = (points[0] - vertex).normalized();
    Vector3 end = (points[2] - vertex).normalized();
    geometry->addGuideAngle(vertex, start, end);
    points.clear();
    previewValid = false;
}

AxesTool::AxesTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void AxesTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;
    anchors.push_back(point);
    if (anchors.size() == 1) {
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
    } else if (anchors.size() == 3) {
        finalizeAxes(point);
        setState(State::Idle);
    } else {
        previewPoint = point;
        previewValid = true;
    }
}

void AxesTool::onPointerMove(const PointerInput& input)
{
    if (anchors.empty())
        return;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void AxesTool::onPointerHover(const PointerInput& input)
{
    if (!anchors.empty()) {
        onPointerMove(input);
        return;
    }
    Vector3 point;
    if (resolvePoint(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else if (resolveFallback(input, point)) {
        previewPoint = point;
        previewValid = true;
    } else {
        previewValid = false;
    }
}

void AxesTool::onCancel()
{
    anchors.clear();
    previewValid = false;
}

void AxesTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        anchors.clear();
        previewValid = false;
    }
}

void AxesTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!anchors.empty() && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    } else if (anchors.empty() && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState AxesTool::buildPreview() const
{
    PreviewState state;
    if (!anchors.empty()) {
        PreviewPolyline polyline;
        polyline.points = anchors;
        if (previewValid)
            polyline.points.push_back(previewPoint);
        state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool AxesTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool AxesTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, out))
        return false;
    axisSnap(out);
    out.y = 0.0f;
    return true;
}

void AxesTool::finalizeAxes(const Vector3& yDirection)
{
    if (!geometry || anchors.size() < 2)
        return;
    Vector3 origin = anchors[0];
    Vector3 xDirection = anchors[1] - origin;
    if (xDirection.lengthSquared() <= 1e-6f)
        xDirection = Vector3(1.0f, 0.0f, 0.0f);
    Vector3 yDir = yDirection - origin;
    geometry->setAxes(origin, xDirection, yDir);
    anchors.clear();
    previewValid = false;
}

