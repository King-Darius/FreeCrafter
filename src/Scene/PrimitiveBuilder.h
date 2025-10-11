#pragma once

#include "GeometryKernel/Vector3.h"

#include <memory>
#include <string>

class GeometryObject;

namespace Scene {

enum class PrimitiveType {
    Box,
    Plane,
    Cylinder,
    Sphere
};

struct PrimitiveOptions {
    PrimitiveType type = PrimitiveType::Box;
    Vector3 center{ 0.0f, 0.0f, 0.0f };
    float width = 1.0f;
    float depth = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    int segments = 32;
    int rings = 16;
    std::string name;
};

std::unique_ptr<GeometryObject> buildPrimitiveGeometry(const PrimitiveOptions& options, std::string* error = nullptr);

std::unique_ptr<GeometryObject> buildPlaneGeometry(float width, float height, float thickness,
                                                   const Vector3& center, std::string* error = nullptr);

std::unique_ptr<GeometryObject> buildSphereGeometry(float radius, int segments, int rings, const Vector3& center,
                                                    std::string* error = nullptr);

} // namespace Scene

