#include "DrawingTools.h"

#include "GroundProjection.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float ix, float iy)
        : x(ix)
        , y(iy)
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

Vec2 toVec2(const Vector3& v)
{
    return { v.x, v.z };
}

Vec2 perpendicular(const Vec2& v)
{
    return { -v.y, v.x };
}

void ensureArcAngles(float& start, float& mid, float& end, bool ccw)
{
    if (ccw) {
        while (mid < start)
            mid += 2.0f * kPi;
        while (end < start)
            end += 2.0f * kPi;
        if (mid > end)
            end += 2.0f * kPi;
    } else {
        while (mid > start)
            mid -= 2.0f * kPi;
        while (end > start)
            end -= 2.0f * kPi;
        if (mid < end)
            end -= 2.0f * kPi;
    }
}

std::vector<Vector3> approximateArc(const Vector3& start, const Vector3& mid, const Vector3& end)
{
    Vec2 a = toVec2(start);
    Vec2 b = toVec2(mid);
    Vec2 c = toVec2(end);

    float d = 2.0f * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    if (std::fabs(d) < 1e-6f) {
        return { start, mid, end };
    }

    float aa = a.x * a.x + a.y * a.y;
    float bb = b.x * b.x + b.y * b.y;
    float cc = c.x * c.x + c.y * c.y;
    float ux = (aa * (b.y - c.y) + bb * (c.y - a.y) + cc * (a.y - b.y)) / d;
    float uy = (aa * (c.x - b.x) + bb * (a.x - c.x) + cc * (b.x - a.x)) / d;
    Vec2 center(ux, uy);
    float radius = (a - center).length();
    if (radius <= 1e-6f) {
        return { start, mid, end };
    }

    bool ccw = ((b - a).cross(c - b)) > 0.0f;
    float startAngle = std::atan2(a.y - uy, a.x - ux);
    float midAngle = std::atan2(b.y - uy, b.x - ux);
    float endAngle = std::atan2(c.y - uy, c.x - ux);
    ensureArcAngles(startAngle, midAngle, endAngle, ccw);

    int segments = std::max(8, static_cast<int>(radius * 16.0f));
    std::vector<Vector3> points;
    points.reserve(static_cast<size_t>(segments) + 1);

    float totalAngle = endAngle - startAngle;
    int steps = std::max(2, segments);
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        float angle = startAngle + totalAngle * t;
        float x = ux + std::cos(angle) * radius;
        float y = uy + std::sin(angle) * radius;
        points.push_back(Vector3(x, start.y, y));
    }
    return points;
}

std::vector<Vector3> buildCircle(const Vector3& center, const Vector3& radiusPoint, int segments)
{
    Vec2 c = toVec2(center);
    Vec2 r = toVec2(radiusPoint);
    float radius = (r - c).length();
    if (radius <= 1e-6f)
        return {};
    std::vector<Vector3> points;
    points.reserve(static_cast<size_t>(segments));
    for (int i = 0; i < segments; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * kPi;
        float x = c.x + std::cos(angle) * radius;
        float y = c.y + std::sin(angle) * radius;
        points.push_back(Vector3(x, center.y, y));
    }
    return points;
}

std::vector<Vector3> buildRegularPolygon(const Vector3& center, const Vector3& radiusPoint, int sides)
{
    if (sides < 3)
        return {};
    Vec2 c = toVec2(center);
    Vec2 r = toVec2(radiusPoint);
    Vec2 offset = r - c;
    float radius = offset.length();
    if (radius <= 1e-6f)
        return {};
    float startAngle = std::atan2(offset.y, offset.x);
    std::vector<Vector3> points;
    points.reserve(static_cast<size_t>(sides));
    for (int i = 0; i < sides; ++i) {
        float angle = startAngle + 2.0f * kPi * static_cast<float>(i) / static_cast<float>(sides);
        float x = c.x + std::cos(angle) * radius;
        float y = c.y + std::sin(angle) * radius;
        points.push_back(Vector3(x, center.y, y));
    }
    return points;
}

std::vector<Vector3> buildRotatedRectangle(const Vector3& first, const Vector3& second, const Vector3& heightPoint)
{
    Vec2 a = toVec2(first);
    Vec2 b = toVec2(second);
    Vec2 h = toVec2(heightPoint);
    Vec2 edge = b - a;
    Vec2 baseDir = edge.normalized();
    if (baseDir.length() <= 1e-6f)
        return { first, second };
    Vec2 perp = perpendicular(baseDir);
    float height = (h - b).dot(perp);
    Vec2 offset = perp * height;
    Vec2 c = b + offset;
    Vec2 d = a + offset;
    return { toVec3(a), toVec3(b), toVec3(c), toVec3(d) };
}

bool maybeAddPoint(std::vector<Vector3>& stroke, const Vector3& point)
{
    if (stroke.empty()) {
        stroke.push_back(point);
        return true;
    }
    Vector3 delta = point - stroke.back();
    if (delta.lengthSquared() < 1e-5f)
        return false;
    stroke.push_back(point);
    return true;
}

using ToolHelpers::axisSnap;
using ToolHelpers::screenToGround;

}

ArcTool::ArcTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void ArcTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    anchors.push_back(point);
    if (anchors.size() == 1) {
        setState(State::Active);
    }

    if (anchors.size() == 3) {
        finalizeArc(point);
        setState(State::Idle);
    } else {
        previewPoint = point;
        previewValid = true;
    }
}

void ArcTool::onPointerMove(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void ArcTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
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

void ArcTool::onCommit()
{
    if (anchors.size() == 3) {
        finalizeArc(anchors.back());
    }
    anchors.clear();
    previewValid = false;
}

void ArcTool::onCancel()
{
    anchors.clear();
    previewValid = false;
}

void ArcTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        anchors.clear();
        previewValid = false;
    }
}

void ArcTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!anchors.empty() && anchors.size() < 3 && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    } else if (anchors.empty() && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState ArcTool::buildPreview() const
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

bool ArcTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool ArcTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void ArcTool::finalizeArc(const Vector3& bulgePoint)
{
    if (!geometry || anchors.size() < 2)
        return;
    std::vector<Vector3> points = approximateArc(anchors[0], anchors[1], bulgePoint);
    if (points.size() >= 2) {
        geometry->addCurve(points);
    }
    anchors.clear();
    previewValid = false;
}

CircleTool::CircleTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void CircleTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    if (!hasCenter) {
        center = point;
        hasCenter = true;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
    } else {
        finalizeCircle(point);
        setState(State::Idle);
    }
}

void CircleTool::onPointerMove(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void CircleTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (hasCenter) {
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

void CircleTool::onCancel()
{
    hasCenter = false;
    previewValid = false;
}

void CircleTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        hasCenter = false;
        previewValid = false;
    }
}

void CircleTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState CircleTool::buildPreview() const
{
    PreviewState state;
    if (hasCenter && previewValid) {
        PreviewPolyline polyline;
        polyline.points = buildCircle(center, previewPoint, segments);
        polyline.closed = true;
        state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool CircleTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool CircleTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void CircleTool::finalizeCircle(const Vector3& radiusPoint)
{
    if (!geometry || !hasCenter)
        return;
    std::vector<Vector3> points = buildCircle(center, radiusPoint, segments);
    if (!points.empty()) {
        geometry->addCurve(points);
    }
    hasCenter = false;
    previewValid = false;
}

PolygonTool::PolygonTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void PolygonTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    if (!hasCenter) {
        center = point;
        hasCenter = true;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
    } else {
        finalizePolygon(point);
        setState(State::Idle);
    }
}

void PolygonTool::onPointerMove(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void PolygonTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (hasCenter) {
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

void PolygonTool::onCancel()
{
    hasCenter = false;
    previewValid = false;
}

void PolygonTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        hasCenter = false;
        previewValid = false;
    }
}

void PolygonTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState PolygonTool::buildPreview() const
{
    PreviewState state;
    if (hasCenter && previewValid) {
        PreviewPolyline polyline;
        polyline.points = buildRegularPolygon(center, previewPoint, sides);
        polyline.closed = true;
        state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool PolygonTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool PolygonTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void PolygonTool::finalizePolygon(const Vector3& radiusPoint)
{
    if (!geometry || !hasCenter)
        return;
    std::vector<Vector3> points = buildRegularPolygon(center, radiusPoint, sides);
    if (!points.empty()) {
        geometry->addCurve(points);
    }
    hasCenter = false;
    previewValid = false;
}

RotatedRectangleTool::RotatedRectangleTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void RotatedRectangleTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    corners.push_back(point);
    if (corners.size() == 1) {
        setState(State::Active);
    } else if (corners.size() == 3) {
        finalizeRectangle(point);
        setState(State::Idle);
    } else {
        previewPoint = point;
        previewValid = true;
    }
}

void RotatedRectangleTool::onPointerMove(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void RotatedRectangleTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (!corners.empty()) {
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

void RotatedRectangleTool::onCancel()
{
    corners.clear();
    previewValid = false;
}

void RotatedRectangleTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        corners.clear();
        previewValid = false;
    }
}

void RotatedRectangleTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!corners.empty() && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    } else if (corners.empty() && result.isValid()) {
        previewPoint = result.position;
        previewValid = true;
    }
}

Tool::PreviewState RotatedRectangleTool::buildPreview() const
{
    PreviewState state;
    if (!corners.empty()) {
        PreviewPolyline polyline;
        polyline.points = corners;
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

bool RotatedRectangleTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool RotatedRectangleTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void RotatedRectangleTool::finalizeRectangle(const Vector3& heightPoint)
{
    if (!geometry || corners.size() < 2)
        return;
    std::vector<Vector3> poly = buildRotatedRectangle(corners[0], corners[1], heightPoint);
    if (poly.size() >= 4) {
        geometry->addCurve(poly);
    }
    corners.clear();
    previewValid = false;
}

FreehandTool::FreehandTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void FreehandTool::onPointerDown(const PointerInput& input)
{
    Vector3 point;
    if (!resolvePoint(input, point))
        return;
    stroke.clear();
    maybeAddPoint(stroke, point);
    drawing = true;
    setState(State::Active);
}

void FreehandTool::onPointerMove(const PointerInput& input)
{
    if (!drawing)
        return;
    Vector3 point;
    if (!resolvePoint(input, point))
        return;
    maybeAddPoint(stroke, point);
}

void FreehandTool::onPointerUp(const PointerInput& input)
{
    if (!drawing)
        return;
    Vector3 point;
    if (resolvePoint(input, point)) {
        maybeAddPoint(stroke, point);
    }
    if (geometry && stroke.size() >= 2) {
        geometry->addCurve(stroke);
    }
    stroke.clear();
    drawing = false;
    setState(State::Idle);
}

void FreehandTool::onCancel()
{
    stroke.clear();
    drawing = false;
}

void FreehandTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        stroke.clear();
        drawing = false;
    }
}

Tool::PreviewState FreehandTool::buildPreview() const
{
    PreviewState state;
    if (!stroke.empty()) {
        PreviewPolyline polyline;
        polyline.points = stroke;
        state.polylines.push_back(polyline);
    }
    return state;
}

bool FreehandTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool FreehandTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

