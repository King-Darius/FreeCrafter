#include "GeometryKernel.h"
#include "Serialization.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>

namespace {
Vector3 safeNormalize(const Vector3& v)
{
    float len = v.length();
    if (len <= 1e-6f) {
        return Vector3(0.0f, 1.0f, 0.0f);
    }
    return v / len;
}
}

GeometryObject* GeometryKernel::addCurve(const std::vector<Vector3>& points) {
    auto obj = Curve::createFromPoints(points);
    if (!obj) {
        return nullptr;
    }
    GeometryObject* raw = obj.get();
    objects.push_back(std::move(obj));
    return raw;
}

GeometryObject* GeometryKernel::extrudeCurve(GeometryObject* curveObj, float height) {
    if (!curveObj || curveObj->getType() != ObjectType::Curve) return nullptr;
    auto* curve = static_cast<Curve*>(curveObj);
    auto obj = Solid::createFromCurve(*curve, height);
    if (!obj) {
        return nullptr;
    }
    GeometryObject* raw = obj.get();
    objects.push_back(std::move(obj));
    return raw;
}

GeometryObject* GeometryKernel::addObject(std::unique_ptr<GeometryObject> object)
{
    if (!object)
        return nullptr;
    GeometryObject* raw = object.get();
    objects.push_back(std::move(object));
    return raw;
}

GeometryObject* GeometryKernel::cloneObject(const GeometryObject& source)
{
    auto clone = source.clone();
    if (!clone)
        return nullptr;
    GeometryObject* raw = clone.get();
    objects.push_back(std::move(clone));
    auto it = materialAssignments.find(&source);
    if (it != materialAssignments.end()) {
        materialAssignments[raw] = it->second;
    }
    auto metaIt = metadataMap.find(&source);
    if (metaIt != metadataMap.end()) {
        metadataMap[raw] = metaIt->second;
    }
    return raw;
}

void GeometryKernel::deleteObject(GeometryObject* obj) {
    if (!obj) return;
    materialAssignments.erase(obj);
    metadataMap.erase(obj);
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->get() == obj) { objects.erase(it); return; }
    }
}

void GeometryKernel::clear()
{
    objects.clear();
    materialAssignments.clear();
    metadataMap.clear();
    textAnnotations.clear();
    dimensions.clear();
    clearGuides();
    resetAxes();
}

bool GeometryKernel::saveToFile(const std::string& filename) const {
    std::ofstream os(filename, std::ios::out | std::ios::trunc);
    if (!os) return false;
    os << "FCM 1\n";
    saveToStream(os);
    return true;
}

void GeometryKernel::saveToStream(std::ostream& os) const
{
    for (const auto& up : objects) {
        if (up->getType() == ObjectType::Curve) {
            GeometryIO::writeCurve(os, *static_cast<const Curve*>(up.get()));
        } else if (up->getType() == ObjectType::Solid) {
            GeometryIO::writeSolid(os, *static_cast<const Solid*>(up.get()));
        }
    }
}

bool GeometryKernel::loadFromFile(const std::string& filename) {
    std::ifstream is(filename);
    if (!is) return false;
    std::string tag; int version=0; is >> tag >> version;
    if (tag!="FCM") return false;
    if (version > 1) {
        std::string token;
        while (is >> token) {
            if (token == "BEGIN_GEOMETRY") {
                loadFromStream(is, "END_GEOMETRY");
                break;
            }
        }
        return true;
    }
    clear();
    while (is) {
        std::string type; if (!(is>>type)) break;
        if (type=="Curve") {
            auto c = GeometryIO::readCurve(is); if (c) objects.push_back(std::move(c));
        } else if (type=="Solid") {
            auto s = GeometryIO::readSolid(is); if (s) objects.push_back(std::move(s));
        } else {
            break;
        }
    }
    return true;
}

void GeometryKernel::loadFromStream(std::istream& is, const std::string& terminator)
{
    clear();
    std::string type;
    while (is >> type) {
        if (type == terminator) {
            break;
        }
        if (type == "Curve") {
            auto curve = GeometryIO::readCurve(is);
            if (curve) {
                objects.push_back(std::move(curve));
            }
        } else if (type == "Solid") {
            auto solid = GeometryIO::readSolid(is);
            if (solid) {
                objects.push_back(std::move(solid));
            }
        } else {
            break;
        }
    }
}

void GeometryKernel::assignMaterial(const GeometryObject* object, const std::string& materialName)
{
    if (!object)
        return;
    if (materialName.empty()) {
        materialAssignments.erase(object);
    } else {
        materialAssignments[object] = materialName;
    }
}

std::string GeometryKernel::getMaterial(const GeometryObject* object) const
{
    if (!object)
        return {};
    auto it = materialAssignments.find(object);
    if (it == materialAssignments.end())
        return {};
    return it->second;
}

void GeometryKernel::setShapeMetadata(const GeometryObject* object, const ShapeMetadata& metadata)
{
    if (!object)
        return;
    if (metadata.type == ShapeMetadata::Type::None) {
        metadataMap.erase(object);
    } else {
        metadataMap[object] = metadata;
    }
}

std::optional<GeometryKernel::ShapeMetadata> GeometryKernel::shapeMetadata(const GeometryObject* object) const
{
    if (!object)
        return std::nullopt;
    auto it = metadataMap.find(object);
    if (it == metadataMap.end())
        return std::nullopt;
    return it->second;
}

bool GeometryKernel::rebuildShapeFromMetadata(GeometryObject* object, const ShapeMetadata& metadata)
{
    if (!object || object->getType() != ObjectType::Curve)
        return false;

    auto normalizeDirection = [](const Vector3& dir) {
        Vector3 normalized = dir;
        if (normalized.lengthSquared() <= 1e-6f)
            normalized = Vector3(1.0f, 0.0f, 0.0f);
        else
            normalized = normalized.normalized();
        return normalized;
    };

    std::vector<Vector3> points;
    ShapeMetadata resolved = metadata;

    switch (metadata.type) {
    case ShapeMetadata::Type::Circle: {
        Vector3 direction = normalizeDirection(metadata.circle.direction);
        float radius = std::max(0.0f, metadata.circle.radius);
        Vector3 radiusPoint = metadata.circle.center + direction * radius;
        int segments = std::max(3, metadata.circle.segments);
        points = ShapeBuilder::buildCircle(metadata.circle.center, radiusPoint, segments);
        resolved.circle.direction = direction;
        resolved.circle.radius = radius;
        resolved.circle.segments = segments;
        break;
    }
    case ShapeMetadata::Type::Polygon: {
        Vector3 direction = normalizeDirection(metadata.polygon.direction);
        float radius = std::max(0.0f, metadata.polygon.radius);
        Vector3 radiusPoint = metadata.polygon.center + direction * radius;
        int sides = std::max(3, metadata.polygon.sides);
        points = ShapeBuilder::buildRegularPolygon(metadata.polygon.center, radiusPoint, sides);
        resolved.polygon.direction = direction;
        resolved.polygon.radius = radius;
        resolved.polygon.sides = sides;
        break;
    }
    case ShapeMetadata::Type::Arc: {
        ShapeBuilder::ArcDefinition def = metadata.arc.definition;
        points = ShapeBuilder::buildArc(def);
        resolved.arc.definition = def;
        break;
    }
    case ShapeMetadata::Type::Bezier: {
        ShapeBuilder::BezierDefinition def = metadata.bezier.definition;
        if (def.segments < 8)
            def.segments = 8;
        points = ShapeBuilder::buildBezier(def);
        resolved.bezier.definition = def;
        break;
    }
    default:
        return false;
    }

    if (points.empty())
        return false;

    auto* curve = static_cast<Curve*>(object);
    if (!curve->rebuildFromPoints(points))
        return false;

    metadataMap[object] = resolved;
    return true;
}

void GeometryKernel::addTextAnnotation(const Vector3& position, std::string text, float height)
{
    if (text.empty())
        return;
    TextAnnotation annotation;
    annotation.position = position;
    annotation.text = std::move(text);
    annotation.height = std::max(0.01f, height);
    textAnnotations.push_back(std::move(annotation));
}

void GeometryKernel::addDimension(const Vector3& start, const Vector3& end)
{
    LinearDimension dim;
    dim.start = start;
    dim.end = end;
    dim.value = (end - start).length();
    dimensions.push_back(dim);
}

void GeometryKernel::addGuideLine(const Vector3& start, const Vector3& end)
{
    GuideState::GuideLine line{ start, end };
    guides.lines.push_back(line);
}

void GeometryKernel::addGuidePoint(const Vector3& position)
{
    guides.points.push_back({ position });
}

void GeometryKernel::addGuideAngle(const Vector3& origin, const Vector3& startDirection, const Vector3& endDirection)
{
    Vector3 s = startDirection.normalized();
    Vector3 e = endDirection.normalized();
    float dot = std::max(-1.0f, std::min(1.0f, s.dot(e)));
    float angle = std::acos(dot) * 180.0f / 3.14159265358979323846f;
    guides.angles.push_back({ origin, s, e, angle });
}

GeometryKernel::MeshBuffer GeometryKernel::buildMeshBuffer(const GeometryObject& object) const
{
    MeshBuffer buffer;
    const HalfEdgeMesh& mesh = object.getMesh();
    const auto& vertices = mesh.getVertices();
    buffer.positions.reserve(vertices.size());
    buffer.normals.assign(vertices.size(), Vector3());
    for (const auto& vertex : vertices) {
        buffer.positions.push_back(vertex.position);
    }

    const auto& triangles = mesh.getTriangles();
    buffer.indices.reserve(triangles.size() * 3);
    for (const auto& tri : triangles) {
        if (tri.v0 < 0 || tri.v1 < 0 || tri.v2 < 0) {
            continue;
        }
        buffer.indices.push_back(static_cast<std::uint32_t>(tri.v0));
        buffer.indices.push_back(static_cast<std::uint32_t>(tri.v1));
        buffer.indices.push_back(static_cast<std::uint32_t>(tri.v2));
        if (static_cast<std::size_t>(tri.v0) < buffer.normals.size()) {
            buffer.normals[static_cast<std::size_t>(tri.v0)] += tri.normal;
        }
        if (static_cast<std::size_t>(tri.v1) < buffer.normals.size()) {
            buffer.normals[static_cast<std::size_t>(tri.v1)] += tri.normal;
        }
        if (static_cast<std::size_t>(tri.v2) < buffer.normals.size()) {
            buffer.normals[static_cast<std::size_t>(tri.v2)] += tri.normal;
        }
    }

    if (buffer.indices.empty() && !vertices.empty()) {
        // Fallback to constructing triangles from faces if triangulation data is missing
        const auto& faces = mesh.getFaces();
        for (const auto& face : faces) {
            std::vector<std::uint32_t> loop;
            int he = face.halfEdge;
            if (he < 0) {
                continue;
            }
            const auto& halfEdges = mesh.getHalfEdges();
            int start = he;
            do {
                const auto& record = halfEdges[he];
                loop.push_back(static_cast<std::uint32_t>(record.origin));
                he = record.next;
            } while (he >= 0 && he != start && loop.size() < vertices.size());
            if (loop.size() < 3) {
                continue;
            }
            for (std::size_t i = 1; i + 1 < loop.size(); ++i) {
                buffer.indices.push_back(loop[0]);
                buffer.indices.push_back(loop[i]);
                buffer.indices.push_back(loop[i + 1]);
            }
        }
    }

    for (auto& normal : buffer.normals) {
        normal = safeNormalize(normal);
    }
    if (buffer.normals.size() != buffer.positions.size()) {
        buffer.normals.assign(buffer.positions.size(), Vector3(0.0f, 1.0f, 0.0f));
    }
    return buffer;
}

std::array<float, 16> GeometryKernel::identityTransform()
{
    return { 1.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 1.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 1.0f };
}

HalfEdgeMesh GeometryKernel::meshFromIndexedData(const std::vector<Vector3>& positions,
                                                 const std::vector<std::uint32_t>& indices)
{
    HalfEdgeMesh mesh;
    for (const auto& pos : positions) {
        mesh.addVertex(pos);
    }
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        std::uint32_t i0 = indices[i];
        std::uint32_t i1 = indices[i + 1];
        std::uint32_t i2 = indices[i + 2];
        if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size()) {
            continue;
        }
        std::vector<int> face = {
            static_cast<int>(i0),
            static_cast<int>(i1),
            static_cast<int>(i2)
        };
        mesh.addFace(face);
    }
    mesh.heal();
    return mesh;
}

void GeometryKernel::clearGuides()
{
    guides.lines.clear();
    guides.points.clear();
    guides.angles.clear();
}

void GeometryKernel::setAxes(const Vector3& origin, const Vector3& xDirection, const Vector3& yDirection)
{
    Vector3 x = xDirection.lengthSquared() > 1e-8f ? xDirection.normalized() : Vector3(1.0f, 0.0f, 0.0f);
    Vector3 y = yDirection.lengthSquared() > 1e-8f ? yDirection.normalized() : Vector3(0.0f, 1.0f, 0.0f);
    Vector3 z = x.cross(y);
    if (z.lengthSquared() <= 1e-6f) {
        z = Vector3(0.0f, 0.0f, 1.0f);
        y = z.cross(x).normalized();
    } else {
        z = z.normalized();
        y = z.cross(x).normalized();
    }
    axes.origin = origin;
    axes.xAxis = x;
    axes.yAxis = y;
    axes.zAxis = z;
    axes.valid = true;
}

void GeometryKernel::resetAxes()
{
    axes = AxesState{};
}
