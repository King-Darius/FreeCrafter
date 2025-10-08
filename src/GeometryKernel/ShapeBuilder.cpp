#include "ShapeBuilder.h"

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
    Vec2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
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

Vec2 toVec2(const Vector3& v)
{
    return { v.x, v.z };
}

float canonicalizeAngle(float angle)
{
    while (angle < 0.0f)
        angle += 2.0f * kPi;
    while (angle >= 2.0f * kPi)
        angle -= 2.0f * kPi;
    return angle;
}

float computeSweep(float start, float end, bool ccw)
{
    if (ccw) {
        float sweep = end - start;
        if (sweep < 0.0f)
            sweep += 2.0f * kPi;
        return sweep;
    }
    float sweep = start - end;
    if (sweep < 0.0f)
        sweep += 2.0f * kPi;
    return -sweep;
}

int defaultSegmentCount(float radius, float sweep)
{
    float effectiveRadius = std::max(0.001f, radius);
    float normalizedSweep = std::fabs(sweep) / (2.0f * kPi);
    int base = std::max(8, static_cast<int>(effectiveRadius * 16.0f * std::max(0.25f, normalizedSweep)));
    return std::max(12, base);
}

} // namespace

namespace ShapeBuilder {

float angleBetween(const Vector3& center, const Vector3& point)
{
    Vec2 c = toVec2(center);
    Vec2 p = toVec2(point);
    Vec2 delta = p - c;
    return std::atan2(delta.y, delta.x);
}

std::vector<Vector3> buildCircle(const Vector3& center, const Vector3& radiusPoint, int segments)
{
    if (segments < 3)
        segments = 3;
    Vec2 c = toVec2(center);
    Vec2 r = toVec2(radiusPoint);
    Vec2 offset = r - c;
    float radius = offset.length();
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

std::vector<Vector3> buildArc(const ArcDefinition& definition)
{
    ArcDefinition def = definition;
    def.startAngle = canonicalizeAngle(def.startAngle);
    def.endAngle = canonicalizeAngle(def.endAngle);
    float sweep = computeSweep(def.startAngle, def.endAngle, def.counterClockwise);
    if (std::fabs(def.radius) <= 1e-6f || std::fabs(sweep) <= 1e-6f)
        return {};
    int segments = def.segments > 0 ? def.segments : defaultSegmentCount(def.radius, sweep);
    segments = std::max(8, segments);
    std::vector<Vector3> points;
    points.reserve(static_cast<size_t>(segments) + 1);
    Vec2 c = toVec2(def.center);
    float step = sweep / static_cast<float>(segments);
    for (int i = 0; i <= segments; ++i) {
        float angle = def.startAngle + step * static_cast<float>(i);
        float x = c.x + std::cos(angle) * def.radius;
        float y = c.y + std::sin(angle) * def.radius;
        points.push_back(Vector3(x, def.center.y, y));
    }
    return points;
}

bool solveArcThroughPoints(const Vector3& start, const Vector3& mid, const Vector3& end, ArcDefinition& outDefinition)
{
    Vec2 a = toVec2(start);
    Vec2 b = toVec2(mid);
    Vec2 c = toVec2(end);
    float d = 2.0f * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    if (std::fabs(d) < 1e-6f)
        return false;

    float aa = a.x * a.x + a.y * a.y;
    float bb = b.x * b.x + b.y * b.y;
    float cc = c.x * c.x + c.y * c.y;
    float ux = (aa * (b.y - c.y) + bb * (c.y - a.y) + cc * (a.y - b.y)) / d;
    float uy = (aa * (c.x - b.x) + bb * (a.x - c.x) + cc * (b.x - a.x)) / d;
    Vec2 center(ux, uy);
    float radius = (a - center).length();
    if (radius <= 1e-6f)
        return false;

    Vec2 ab = b - a;
    Vec2 bc = c - b;
    bool ccw = (ab.cross(bc) > 0.0f);

    float startAngle = std::atan2(a.y - uy, a.x - ux);
    float midAngle = std::atan2(b.y - uy, b.x - ux);
    float endAngle = std::atan2(c.y - uy, c.x - ux);

    ArcDefinition def;
    def.center = Vector3(center.x, start.y, center.y);
    def.radius = radius;
    def.counterClockwise = ccw;
    def.startAngle = canonicalizeAngle(startAngle);
    def.endAngle = canonicalizeAngle(endAngle);

    float sweep = computeSweep(def.startAngle, midAngle, def.counterClockwise);
    float sweep2 = computeSweep(midAngle, def.endAngle, def.counterClockwise);
    if (def.counterClockwise) {
        if (sweep < 0.0f)
            sweep += 2.0f * kPi;
        if (sweep2 < 0.0f)
            sweep2 += 2.0f * kPi;
    } else {
        if (sweep > 0.0f)
            sweep -= 2.0f * kPi;
        if (sweep2 > 0.0f)
            sweep2 -= 2.0f * kPi;
    }
    float totalSweep = sweep + sweep2;
    def.segments = defaultSegmentCount(def.radius, totalSweep);

    outDefinition = def;
    return true;
}

ArcDefinition makeArcFromCenter(const Vector3& center, float radius, float startAngle, float endAngle, bool counterClockwise, int segmentsHint)
{
    ArcDefinition def;
    def.center = center;
    def.radius = std::max(0.0f, radius);
    def.startAngle = canonicalizeAngle(startAngle);
    def.endAngle = canonicalizeAngle(endAngle);
    def.counterClockwise = counterClockwise;
    float sweep = computeSweep(def.startAngle, def.endAngle, counterClockwise);
    if (segmentsHint > 0) {
        def.segments = segmentsHint;
    } else {
        def.segments = defaultSegmentCount(def.radius, sweep);
    }
    return def;
}

std::vector<Vector3> buildBezier(const BezierDefinition& definition)
{
    const int segments = std::max(8, definition.segments);
    std::vector<Vector3> points;
    points.reserve(static_cast<size_t>(segments) + 1);
    auto lerp = [](const Vector3& a, const Vector3& b, float t) {
        return a * (1.0f - t) + b * t;
    };
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        Vector3 p01 = lerp(definition.p0, definition.h0, t);
        Vector3 p12 = lerp(definition.h0, definition.h1, t);
        Vector3 p23 = lerp(definition.h1, definition.p1, t);
        Vector3 p012 = lerp(p01, p12, t);
        Vector3 p123 = lerp(p12, p23, t);
        Vector3 p = lerp(p012, p123, t);
        points.push_back(p);
    }
    return points;
}

} // namespace ShapeBuilder

