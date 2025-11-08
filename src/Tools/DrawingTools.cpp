#include "DrawingTools.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "GroundProjection.h"
#include "ToolCommands.h"
#include "../Core/CommandStack.h"
#include "../GeometryKernel/ShapeBuilder.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <QString>

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

std::vector<Vector3> buildAxisAlignedRectangle(const Vector3& first, const Vector3& opposite)
{
    float width = std::fabs(opposite.x - first.x);
    float depth = std::fabs(opposite.z - first.z);
    if (width <= 1e-6f || depth <= 1e-6f)
        return {};

    const float y = first.y;
    Vector3 a = first;
    Vector3 b(opposite.x, y, first.z);
    Vector3 c(opposite.x, y, opposite.z);
    Vector3 d(first.x, y, opposite.z);
    return { a, b, c, d };
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

bool solveTangentArc(const Vector3& start, const Vector3& tangentRef, const Vector3& end, ShapeBuilder::ArcDefinition& out)
{
    Vec2 s = toVec2(start);
    Vec2 e = toVec2(end);
    Vec2 tanPoint = toVec2(tangentRef);
    Vec2 tangent = (tanPoint - s).normalized();
    if (tangent.length() <= 1e-6f)
        return false;

    Vec2 normal(-tangent.y, tangent.x);
    Vec2 diff = s - e;
    float diffLenSq = diff.dot(diff);

    auto trySolve = [&](const Vec2& n) -> bool {
        float denom = 2.0f * n.dot(diff);
        if (std::fabs(denom) <= 1e-6f)
            return false;
        float t = -diffLenSq / denom;
        Vec2 center = s + n * t;
        float radius = (center - s).length();
        if (radius <= 1e-6f)
            return false;
        Vector3 center3(center.x, start.y, center.y);
        float startAngle = std::atan2(s.y - center.y, s.x - center.x);
        float endAngle = std::atan2(e.y - center.y, e.x - center.x);
        bool ccw = ((s - center).cross(e - center) > 0.0f);
        out = ShapeBuilder::makeArcFromCenter(center3, radius, startAngle, endAngle, ccw, 0);
        return true;
    };

    if (trySolve(normal))
        return true;
    return trySolve(normal * -1.0f);
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
    if (anchors.size() >= 2 && previewValid) {
        ShapeBuilder::ArcDefinition def;
        if (ShapeBuilder::solveArcThroughPoints(anchors[0], anchors[1], previewPoint, def)) {
            PreviewPolyline polyline;
            polyline.points = ShapeBuilder::buildArc(def);
            state.polylines.push_back(polyline);
            return state;
        }
    }

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
    ShapeBuilder::ArcDefinition def;
    std::vector<Vector3> points;
    bool solved = ShapeBuilder::solveArcThroughPoints(anchors[0], anchors[1], bulgePoint, def);
    if (solved)
        points = ShapeBuilder::buildArc(def);
    if (points.empty()) {
        points = { anchors[0], anchors[1], bulgePoint };
    }
    if (points.size() >= 2) {
        if (GeometryObject* object = geometry->addCurve(points)) {
            if (solved) {
                GeometryKernel::ShapeMetadata metadata;
                metadata.type = GeometryKernel::ShapeMetadata::Type::Arc;
                metadata.arc.definition = def;
                geometry->setShapeMetadata(object, metadata);
            }
        }
    }
    anchors.clear();
    previewValid = false;
}

CenterArcTool::CenterArcTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void CenterArcTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    switch (stage) {
    case Stage::Center:
        center = point;
        stage = Stage::Start;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
        break;
    case Stage::Start:
        startPoint = point;
        stage = Stage::End;
        previewPoint = point;
        previewValid = true;
        break;
    case Stage::End:
        finalizeArc(point);
        setState(State::Idle);
        break;
    }
}

void CenterArcTool::onPointerMove(const PointerInput& input)
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

void CenterArcTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (stage != Stage::Center) {
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

void CenterArcTool::onCancel()
{
    stage = Stage::Center;
    previewValid = false;
}

void CenterArcTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        stage = Stage::Center;
        previewValid = false;
    }
}

void CenterArcTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!result.isValid())
        return;
    previewPoint = result.position;
    previewValid = true;
}

Tool::PreviewState CenterArcTool::buildPreview() const
{
    PreviewState state;
    if (stage == Stage::Start) {
        if (previewValid) {
            PreviewPolyline line;
            line.points = { center, previewPoint };
            state.polylines.push_back(line);
        }
    } else if (stage == Stage::End && previewValid) {
        Vector3 radiusVec = startPoint - center;
        if (radiusVec.lengthSquared() > 1e-6f) {
            Vec2 startVec = toVec2(startPoint) - toVec2(center);
            Vec2 endVec = toVec2(previewPoint) - toVec2(center);
            float radius = std::sqrt(radiusVec.lengthSquared());
            bool ccw = (startVec.cross(endVec) > 0.0f);
            float startAngle = std::atan2(startVec.y, startVec.x);
            float endAngle = std::atan2(endVec.y, endVec.x);
            ShapeBuilder::ArcDefinition def = ShapeBuilder::makeArcFromCenter(center, radius, startAngle, endAngle, ccw, 0);
            PreviewPolyline arcPolyline;
            arcPolyline.points = ShapeBuilder::buildArc(def);
            state.polylines.push_back(arcPolyline);
        }
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool CenterArcTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool CenterArcTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void CenterArcTool::finalizeArc(const Vector3& endPoint)
{
    if (!geometry || stage != Stage::End)
        return;
    Vector3 radiusVec = startPoint - center;
    if (radiusVec.lengthSquared() <= 1e-6f)
        return;
    Vec2 startVec = toVec2(startPoint) - toVec2(center);
    Vec2 endVec = toVec2(endPoint) - toVec2(center);
    if (endVec.length() <= 1e-6f)
        return;
    float radius = std::sqrt(radiusVec.lengthSquared());
    bool ccw = (startVec.cross(endVec) > 0.0f);
    float startAngle = std::atan2(startVec.y, startVec.x);
    float endAngle = std::atan2(endVec.y, endVec.x);
    ShapeBuilder::ArcDefinition def = ShapeBuilder::makeArcFromCenter(center, radius, startAngle, endAngle, ccw, 0);
    std::vector<Vector3> points = ShapeBuilder::buildArc(def);
    if (points.size() >= 2) {
        if (GeometryObject* object = geometry->addCurve(points)) {
            GeometryKernel::ShapeMetadata metadata;
            metadata.type = GeometryKernel::ShapeMetadata::Type::Arc;
            metadata.arc.definition = def;
            geometry->setShapeMetadata(object, metadata);
        }
    }
    stage = Stage::Center;
    previewValid = false;
}

TangentArcTool::TangentArcTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void TangentArcTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    switch (stage) {
    case Stage::Start:
        startPoint = point;
        stage = Stage::Tangent;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
        break;
    case Stage::Tangent:
        tangentReference = point;
        stage = Stage::End;
        previewPoint = point;
        previewValid = true;
        break;
    case Stage::End:
        finalizeArc(point);
        setState(State::Idle);
        break;
    }
}

void TangentArcTool::onPointerMove(const PointerInput& input)
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

void TangentArcTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (stage != Stage::Start) {
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

void TangentArcTool::onCancel()
{
    stage = Stage::Start;
    previewValid = false;
}

void TangentArcTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        stage = Stage::Start;
        previewValid = false;
    }
}

void TangentArcTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!result.isValid())
        return;
    previewPoint = result.position;
    previewValid = true;
}

Tool::PreviewState TangentArcTool::buildPreview() const
{
    PreviewState state;
    if (stage == Stage::Tangent && previewValid) {
        PreviewPolyline line;
        line.points = { startPoint, previewPoint };
        state.polylines.push_back(line);
    } else if (stage == Stage::End && previewValid) {
        ShapeBuilder::ArcDefinition def;
        if (solveTangentArc(startPoint, tangentReference, previewPoint, def)) {
            PreviewPolyline arcPolyline;
            arcPolyline.points = ShapeBuilder::buildArc(def);
            state.polylines.push_back(arcPolyline);
        }
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool TangentArcTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool TangentArcTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void TangentArcTool::finalizeArc(const Vector3& endPoint)
{
    if (!geometry || stage != Stage::End)
        return;
    ShapeBuilder::ArcDefinition def;
    bool solved = solveTangentArc(startPoint, tangentReference, endPoint, def);
    std::vector<Vector3> points;
    if (solved)
        points = ShapeBuilder::buildArc(def);
    if (points.empty())
        points = { startPoint, endPoint };
    if (points.size() >= 2) {
        if (GeometryObject* object = geometry->addCurve(points)) {
            if (solved) {
                GeometryKernel::ShapeMetadata metadata;
                metadata.type = GeometryKernel::ShapeMetadata::Type::Arc;
                metadata.arc.definition = def;
                geometry->setShapeMetadata(object, metadata);
            }
        }
    }
    stage = Stage::Start;
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
        polyline.points = ShapeBuilder::buildCircle(center, previewPoint, segments);
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
    std::vector<Vector3> points = ShapeBuilder::buildCircle(center, radiusPoint, segments);
    if (!points.empty()) {
        if (GeometryObject* object = geometry->addCurve(points)) {
            Vector3 direction = radiusPoint - center;
            float radius = std::sqrt(std::max(0.0f, direction.lengthSquared()));
            if (radius > 1e-6f) {
                GeometryKernel::ShapeMetadata metadata;
                metadata.type = GeometryKernel::ShapeMetadata::Type::Circle;
                metadata.circle.center = center;
                metadata.circle.direction = direction.normalized();
                metadata.circle.radius = radius;
                metadata.circle.segments = segments;
                geometry->setShapeMetadata(object, metadata);
            }
        }
    }
    hasCenter = false;
    previewValid = false;
}

RectangleTool::RectangleTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void RectangleTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    if (!hasFirstCorner) {
        firstCorner = point;
        hasFirstCorner = true;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
    } else {
        finalizeRectangle(point);
        setState(State::Idle);
    }
}

void RectangleTool::onPointerMove(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (!hasFirstCorner)
        return;

    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void RectangleTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (hasFirstCorner) {
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

void RectangleTool::onCancel()
{
    hasFirstCorner = false;
    previewValid = false;
}

void RectangleTool::onStateChanged(State, State next)
{
    if (next == State::Idle) {
        hasFirstCorner = false;
        previewValid = false;
    }
}

void RectangleTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!result.isValid())
        return;

    previewPoint = result.position;
    previewValid = true;
}

Tool::PreviewState RectangleTool::buildPreview() const
{
    PreviewState state;
    if (hasFirstCorner && previewValid) {
        PreviewPolyline polyline;
        polyline.points = buildAxisAlignedRectangle(firstCorner, previewPoint);
        polyline.closed = true;
        if (!polyline.points.empty())
            state.polylines.push_back(polyline);
    } else if (previewValid) {
        PreviewPolyline dot;
        dot.points.push_back(previewPoint);
        state.polylines.push_back(dot);
    }
    return state;
}

bool RectangleTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool RectangleTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void RectangleTool::finalizeRectangle(const Vector3& oppositeCorner)
{
    if (!geometry || !hasFirstCorner)
        return;

    std::vector<Vector3> polyline = buildAxisAlignedRectangle(firstCorner, oppositeCorner);
    if (polyline.empty())
        return;

    if (auto* stack = getCommandStack()) {
        auto command = std::make_unique<Tools::CreateCurveCommand>(polyline, QStringLiteral("Draw Rectangle"));
        stack->push(std::move(command));
    } else if (geometry) {
        geometry->addCurve(polyline);
    }

    hasFirstCorner = false;
    previewValid = false;
}

PolygonTool::PolygonTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

Tool::MeasurementKind PolygonTool::getMeasurementKind() const
{
    return Tool::MeasurementKind::Count;
}

Tool::OverrideResult PolygonTool::applyMeasurementOverride(double value)
{
    int requested = static_cast<int>(std::lround(value));
    if (requested < 3)
        requested = 3;
    if (requested > 128)
        requested = 128;
    if (requested == sides)
        return Tool::OverrideResult::PreviewUpdated;
    sides = requested;
    return Tool::OverrideResult::PreviewUpdated;
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
        polyline.points = ShapeBuilder::buildRegularPolygon(center, previewPoint, sides);
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
    std::vector<Vector3> points = ShapeBuilder::buildRegularPolygon(center, radiusPoint, sides);
    if (!points.empty()) {
        if (GeometryObject* object = geometry->addCurve(points)) {
            Vector3 direction = radiusPoint - center;
            float radius = std::sqrt(std::max(0.0f, direction.lengthSquared()));
            if (radius > 1e-6f) {
                GeometryKernel::ShapeMetadata metadata;
                metadata.type = GeometryKernel::ShapeMetadata::Type::Polygon;
                metadata.polygon.center = center;
                metadata.polygon.direction = direction.normalized();
                metadata.polygon.radius = radius;
                metadata.polygon.sides = sides;
                geometry->setShapeMetadata(object, metadata);
            }
        }
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

BezierTool::BezierTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
}

void BezierTool::onPointerDown(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    Vector3 point;
    if (!resolvePoint(input, point)) {
        previewValid = false;
        return;
    }

    switch (stage) {
    case Stage::FirstAnchor:
        firstAnchor = point;
        hasFirstAnchor = true;
        stage = Stage::SecondAnchor;
        previewPoint = point;
        previewValid = true;
        setState(State::Active);
        break;
    case Stage::SecondAnchor:
        if (!hasFirstAnchor)
            return;
        secondAnchor = point;
        hasSecondAnchor = true;
        stage = Stage::FirstHandle;
        previewPoint = point;
        previewValid = true;
        break;
    case Stage::FirstHandle:
        if (!hasFirstAnchor || !hasSecondAnchor)
            return;
        firstHandle = point;
        hasFirstHandle = true;
        stage = Stage::SecondHandle;
        previewPoint = point;
        previewValid = true;
        break;
    case Stage::SecondHandle:
        finalizeCurve(point);
        setState(State::Idle);
        break;
    }
}

void BezierTool::onPointerMove(const PointerInput& input)
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

void BezierTool::onPointerHover(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    if (stage != Stage::FirstAnchor && stage != Stage::SecondAnchor) {
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

void BezierTool::onCancel()
{
    stage = Stage::FirstAnchor;
    hasFirstAnchor = false;
    hasSecondAnchor = false;
    hasFirstHandle = false;
    previewValid = false;
}

void BezierTool::onStateChanged(State, State next)
{
    if (next == State::Idle)
        onCancel();
}

void BezierTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!result.isValid())
        return;
    previewPoint = result.position;
    previewValid = true;
}

Tool::PreviewState BezierTool::buildPreview() const
{
    PreviewState state;
    if (!hasFirstAnchor) {
        if (previewValid) {
            PreviewPolyline dot;
            dot.points.push_back(previewPoint);
            state.polylines.push_back(dot);
        }
        return state;
    }

    PreviewPolyline control;
    control.points.push_back(firstAnchor);
    if (stage == Stage::FirstHandle && previewValid) {
        control.points.push_back(previewPoint);
    } else if (hasFirstHandle) {
        control.points.push_back(firstHandle);
    }
    if (hasSecondAnchor)
        control.points.push_back(secondAnchor);
    else if (stage == Stage::SecondAnchor && previewValid)
        control.points.push_back(previewPoint);
    if (control.points.size() >= 2)
        state.polylines.push_back(control);

    if (stage == Stage::SecondHandle && previewValid && hasFirstHandle && hasSecondAnchor) {
        ShapeBuilder::BezierDefinition def;
        def.p0 = firstAnchor;
        def.h0 = firstHandle;
        def.h1 = previewPoint;
        def.p1 = secondAnchor;
        def.segments = std::max(16, static_cast<int>(std::ceil((secondAnchor - firstAnchor).length() * 8.0f)));
        PreviewPolyline curve;
        curve.points = ShapeBuilder::buildBezier(def);
        state.polylines.push_back(curve);
    }

    return state;
}

bool BezierTool::resolvePoint(const PointerInput& input, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(input, out);
}

bool BezierTool::resolveFallback(const PointerInput& input, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, input.x, input.y, viewportWidth, viewportHeight, ground))
        return false;
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

void BezierTool::finalizeCurve(const Vector3& handle)
{
    if (!geometry || !hasFirstAnchor || !hasSecondAnchor || !hasFirstHandle)
        return;
    ShapeBuilder::BezierDefinition def;
    def.p0 = firstAnchor;
    def.h0 = firstHandle;
    def.h1 = handle;
    def.p1 = secondAnchor;
    def.segments = std::max(16, static_cast<int>(std::ceil((secondAnchor - firstAnchor).length() * 8.0f)));
    std::vector<Vector3> points = ShapeBuilder::buildBezier(def);
    if (!points.empty()) {
        if (GeometryObject* object = geometry->addCurve(points)) {
            GeometryKernel::ShapeMetadata metadata;
            metadata.type = GeometryKernel::ShapeMetadata::Type::Bezier;
            metadata.bezier.definition = def;
            geometry->setShapeMetadata(object, metadata);
        }
    }
    stage = Stage::FirstAnchor;
    hasFirstAnchor = false;
    hasSecondAnchor = false;
    hasFirstHandle = false;
    previewValid = false;
}

