#include "ToolGeometryUtils.h"

#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"
#include "../GeometryKernel/TransformUtils.h"
#include "../GeometryKernel/HalfEdgeMesh.h"

#include <cmath>

#include <algorithm>

namespace {

BoundingBox boxFromVertices(const std::vector<Vector3>& vertices)
{
    BoundingBox box;
    if (vertices.empty()) return box;
    box.min = vertices.front();
    box.max = vertices.front();
    box.valid = true;
    for (const auto& v : vertices) {
        box.min.x = std::min(box.min.x, v.x);
        box.min.y = std::min(box.min.y, v.y);
        box.min.z = std::min(box.min.z, v.z);
        box.max.x = std::max(box.max.x, v.x);
        box.max.y = std::max(box.max.y, v.y);
        box.max.z = std::max(box.max.z, v.z);
    }
    return box;
}

std::vector<Vector3> meshVertices(const HalfEdgeMesh& mesh)
{
    std::vector<Vector3> verts;
    verts.reserve(mesh.getVertices().size());
    for (const auto& v : mesh.getVertices()) {
        verts.push_back(v.position);
    }
    return verts;
}

}

BoundingBox computeBoundingBox(const GeometryObject& object)
{
    BoundingBox box;
    if (object.getType() == ObjectType::Curve) {
        const Curve& curve = static_cast<const Curve&>(object);
        box = boxFromVertices(curve.getBoundaryLoop());
    } else {
        const HalfEdgeMesh& mesh = object.getMesh();
        box = boxFromVertices(meshVertices(mesh));
    }
    return box;
}

Vector3 computeCentroid(const GeometryObject& object)
{
    BoundingBox box = computeBoundingBox(object);
    if (!box.valid) {
        return Vector3();
    }
    return Vector3((box.min.x + box.max.x) * 0.5f, (box.min.y + box.max.y) * 0.5f, (box.min.z + box.max.z) * 0.5f);
}

void translateObject(GeometryObject& object, const Vector3& delta)
{
    if (object.getType() == ObjectType::Curve) {
        static_cast<Curve&>(object).translate(delta);
    } else {
        static_cast<Solid&>(object).translate(delta);
    }
}

void rotateObject(GeometryObject& object, const Vector3& pivot, const Vector3& axis, float angleRadians)
{
    if (object.getType() == ObjectType::Curve) {
        static_cast<Curve&>(object).rotate(pivot, axis, angleRadians);
    } else {
        static_cast<Solid&>(object).rotate(pivot, axis, angleRadians);
    }
}

void scaleObject(GeometryObject& object, const Vector3& pivot, const Vector3& factors)
{
    if (object.getType() == ObjectType::Curve) {
        static_cast<Curve&>(object).scale(pivot, factors);
    } else {
        static_cast<Solid&>(object).scale(pivot, factors);
    }
}

bool projectScreenToGround(CameraController* camera, int x, int y, int viewportWidth, int viewportHeight, Vector3& out)
{
    if (!camera || viewportWidth <= 0 || viewportHeight <= 0)
        return false;

    float cx, cy, cz;
    camera->getCameraPosition(cx, cy, cz);
    float yaw = camera->getYaw();
    float pitch = camera->getPitch();
    float ry = yaw * static_cast<float>(M_PI) / 180.0f;
    float rp = pitch * static_cast<float>(M_PI) / 180.0f;

    Vector3 forward(-sinf(ry) * cosf(rp), -sinf(rp), -cosf(ry) * cosf(rp));
    forward = forward.normalized();
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = forward.cross(up).normalized();
    up = right.cross(forward).normalized();

    const float fov = 60.0f;
    float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    float nx = (2.0f * static_cast<float>(x) / static_cast<float>(viewportWidth)) - 1.0f;
    float ny = 1.0f - (2.0f * static_cast<float>(y) / static_cast<float>(viewportHeight));
    float tanHalf = tanf((fov * static_cast<float>(M_PI) / 180.0f) / 2.0f);
    Vector3 dir = (forward + right * (nx * tanHalf * aspect) + up * (ny * tanHalf)).normalized();

    Vector3 origin(cx, cy, cz);
    if (std::fabs(dir.y) < 1e-6f)
        return false;
    float t = -origin.y / dir.y;
    if (t < 0.0f)
        return false;
    out = origin + dir * t;
    return true;
}

Vector3 snapPlanarToGrid(const Vector3& point, float grid, float epsilon)
{
    Vector3 snapped = point;
    if (grid <= 0.0f)
        return snapped;
    float gx = std::round(point.x / grid) * grid;
    float gz = std::round(point.z / grid) * grid;
    if (std::fabs(point.x - gx) < epsilon)
        snapped.x = gx;
    if (std::fabs(point.z - gz) < epsilon)
        snapped.z = gz;
    return snapped;
}

bool resolvePlanarPoint(const Interaction::InferenceResult& snap,
    CameraController* camera,
    int x,
    int y,
    int viewportWidth,
    int viewportHeight,
    Vector3& out)
{
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }

    Vector3 ground;
    if (!projectScreenToGround(camera, x, y, viewportWidth, viewportHeight, ground))
        return false;

    ground = snapPlanarToGrid(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

namespace {

struct Vec2
{
    float x = 0.0f;
    float z = 0.0f;
};

Vec2 toVec2(const Vector3& value)
{
    return Vec2{ value.x, value.z };
}

Vector3 fromVec2(const Vec2& value, float y)
{
    return Vector3(value.x, y, value.z);
}

float length(const Vec2& value)
{
    return std::sqrt(value.x * value.x + value.z * value.z);
}

Vec2 normalize(const Vec2& value)
{
    float len = length(value);
    if (len <= 1e-6f) {
        return Vec2{};
    }
    return Vec2{ value.x / len, value.z / len };
}

Vec2 outwardNormal(const Vec2& direction)
{
    return Vec2{ direction.z, -direction.x };
}

bool intersectLines(const Vec2& p1, const Vec2& d1, const Vec2& p2, const Vec2& d2, Vec2& out)
{
    float det = d1.x * d2.z - d1.z * d2.x;
    if (std::fabs(det) <= 1e-6f)
        return false;
    float t = ((p2.x - p1.x) * d2.z - (p2.z - p1.z) * d2.x) / det;
    if (!std::isfinite(t))
        return false;
    out.x = p1.x + d1.x * t;
    out.z = p1.z + d1.z * t;
    return true;
}

bool pointOnSegment(const Vec2& p, const Vec2& a, const Vec2& b, float tolerance)
{
    Vec2 ab{ b.x - a.x, b.z - a.z };
    Vec2 ap{ p.x - a.x, p.z - a.z };
    float abLenSq = ab.x * ab.x + ab.z * ab.z;
    if (abLenSq <= 1e-6f)
        return (std::fabs(ap.x) <= tolerance && std::fabs(ap.z) <= tolerance);
    float t = (ap.x * ab.x + ap.z * ab.z) / abLenSq;
    if (t < -1e-3f || t > 1.001f)
        return false;
    Vec2 closest{ a.x + ab.x * t, a.z + ab.z * t };
    float dx = closest.x - p.x;
    float dz = closest.z - p.z;
    return (dx * dx + dz * dz) <= tolerance * tolerance;
}

} // namespace

bool pointInPolygonXZ(const std::vector<Vector3>& polygon, const Vector3& point, float tolerance)
{
    if (polygon.size() < 3)
        return false;

    Vec2 p = toVec2(point);
    bool inside = false;
    size_t n = polygon.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        Vec2 pi = toVec2(polygon[i]);
        Vec2 pj = toVec2(polygon[j]);

        if (pointOnSegment(p, pj, pi, tolerance)) {
            return true;
        }

        bool intersect = ((pi.z > p.z) != (pj.z > p.z))
            && (p.x < (pj.x - pi.x) * (p.z - pi.z) / (pj.z - pi.z + 1e-12f) + pi.x);
        if (intersect)
            inside = !inside;
    }
    return inside;
}

bool offsetPolygon(const std::vector<Vector3>& polygon, float distance, std::vector<Vector3>& out)
{
    out.clear();
    if (polygon.size() < 3)
        return false;

    size_t count = polygon.size();
    out.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const Vector3& prev3 = polygon[(i + count - 1) % count];
        const Vector3& curr3 = polygon[i];
        const Vector3& next3 = polygon[(i + 1) % count];

        Vec2 prev = toVec2(prev3);
        Vec2 curr = toVec2(curr3);
        Vec2 next = toVec2(next3);

        Vec2 dirPrev = normalize(Vec2{ curr.x - prev.x, curr.z - prev.z });
        Vec2 dirNext = normalize(Vec2{ next.x - curr.x, next.z - curr.z });

        if (length(dirPrev) <= 1e-6f || length(dirNext) <= 1e-6f) {
            out.push_back(curr3);
            continue;
        }

        Vec2 normalPrev = outwardNormal(dirPrev);
        Vec2 normalNext = outwardNormal(dirNext);

        Vec2 linePointA{ curr.x + normalPrev.x * distance, curr.z + normalPrev.z * distance };
        Vec2 linePointB{ curr.x + normalNext.x * distance, curr.z + normalNext.z * distance };

        Vec2 intersection;
        if (!intersectLines(linePointA, dirPrev, linePointB, dirNext, intersection)) {
            Vec2 bisector{ normalPrev.x + normalNext.x, normalPrev.z + normalNext.z };
            float bisLength = length(bisector);
            if (bisLength <= 1e-6f) {
                bisector = normalPrev;
                bisLength = length(bisector);
            }
            if (bisLength <= 1e-6f) {
                intersection = linePointA;
            } else {
                float scale = distance / std::max(bisLength, 1e-6f);
                intersection.x = curr.x + bisector.x * scale;
                intersection.z = curr.z + bisector.z * scale;
            }
        }

        out.push_back(fromVec2(intersection, curr3.y));
    }
    return out.size() == count;
}

