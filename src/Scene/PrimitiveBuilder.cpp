#include "PrimitiveBuilder.h"

#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/ShapeBuilder.h"
#include "GeometryKernel/Solid.h"

#include <cstdint>
#include <cmath>
#include <vector>

namespace Scene {
namespace {
constexpr float kDefaultPlaneThickness = 0.01f;

std::unique_ptr<GeometryObject> buildBox(const PrimitiveOptions& options, std::string* error)
{
    float w = std::max(0.001f, options.width);
    float d = std::max(0.001f, options.depth);
    float h = std::max(0.001f, options.height);

    std::vector<Vector3> base = {
        Vector3(-w * 0.5f, 0.0f, -d * 0.5f),
        Vector3( w * 0.5f, 0.0f, -d * 0.5f),
        Vector3( w * 0.5f, 0.0f,  d * 0.5f),
        Vector3(-w * 0.5f, 0.0f,  d * 0.5f)
    };

    auto solid = Solid::createFromProfile(base, h);
    if (!solid) {
        if (error)
            *error = "Failed to construct box geometry";
        return nullptr;
    }
    solid->translate(Vector3(options.center.x, options.center.y - h * 0.5f, options.center.z));
    return solid;
}

std::unique_ptr<GeometryObject> buildCylinder(const PrimitiveOptions& options, std::string* error)
{
    float radius = std::max(0.001f, options.radius);
    float height = std::max(0.001f, options.height);
    int segments = std::max(8, options.segments);

    auto circle = ShapeBuilder::buildCircle(Vector3(0.0f, 0.0f, 0.0f), Vector3(radius, 0.0f, 0.0f), segments);
    if (circle.empty()) {
        if (error)
            *error = "Failed to build cylinder base circle";
        return nullptr;
    }

    auto solid = Solid::createFromProfile(circle, height);
    if (!solid) {
        if (error)
            *error = "Failed to extrude cylinder";
        return nullptr;
    }

    solid->translate(Vector3(options.center.x, options.center.y - height * 0.5f, options.center.z));
    return solid;
}

std::unique_ptr<GeometryObject> buildSphere(float radius, int segments, int rings, const Vector3& center, std::string* error)
{
    radius = std::max(0.001f, radius);
    segments = std::max(8, segments);
    rings = std::max(4, rings);

    std::vector<Vector3> positions;
    std::vector<std::uint32_t> indices;
    positions.reserve(static_cast<std::size_t>((rings + 1) * (segments + 1)));
    indices.reserve(static_cast<std::size_t>(rings * segments * 6));

    constexpr float kPi = 3.14159265358979323846f;

    for (int ring = 0; ring <= rings; ++ring) {
        float v = static_cast<float>(ring) / static_cast<float>(rings);
        float phi = v * kPi;
        float y = std::cos(phi) * radius;
        float r = std::sin(phi) * radius;
        for (int seg = 0; seg <= segments; ++seg) {
            float u = static_cast<float>(seg) / static_cast<float>(segments);
            float theta = u * kPi * 2.0f;
            float x = std::cos(theta) * r;
            float z = std::sin(theta) * r;
            positions.emplace_back(x, y, z);
        }
    }

    int stride = segments + 1;
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            std::uint32_t i0 = static_cast<std::uint32_t>(ring * stride + seg);
            std::uint32_t i1 = i0 + 1;
            std::uint32_t i2 = i0 + stride;
            std::uint32_t i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    HalfEdgeMesh mesh = GeometryKernel::meshFromIndexedData(positions, indices);
    auto solid = Solid::createFromMesh(std::move(mesh));
    if (!solid) {
        if (error)
            *error = "Failed to build sphere mesh";
        return nullptr;
    }
    solid->translate(center);
    return solid;
}

} // namespace

std::unique_ptr<GeometryObject> buildPrimitiveGeometry(const PrimitiveOptions& options, std::string* error)
{
    switch (options.type) {
    case PrimitiveType::Box:
        return buildBox(options, error);
    case PrimitiveType::Plane:
        return buildPlaneGeometry(options.width, options.depth, kDefaultPlaneThickness, options.center, error);
    case PrimitiveType::Cylinder:
        return buildCylinder(options, error);
    case PrimitiveType::Sphere:
        return buildSphereGeometry(options.radius, options.segments, options.rings, options.center, error);
    default:
        if (error)
            *error = "Unsupported primitive";
        return nullptr;
    }
}

std::unique_ptr<GeometryObject> buildPlaneGeometry(float width, float depth, float thickness,
                                                   const Vector3& center, std::string* error)
{
    PrimitiveOptions opts;
    opts.type = PrimitiveType::Box;
    opts.width = std::max(0.001f, width);
    opts.depth = std::max(0.001f, depth);
    opts.height = std::max(0.0001f, thickness);
    opts.center = center;
    return buildBox(opts, error);
}

std::unique_ptr<GeometryObject> buildSphereGeometry(float radius, int segments, int rings, const Vector3& center,
                                                    std::string* error)
{
    return buildSphere(radius, segments, rings, center, error);
}

} // namespace Scene

