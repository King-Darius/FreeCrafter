#pragma once
#include <memory>
#include <vector>
#include "GeometryObject.h"
#include "Vector3.h"

class Curve;

class Solid : public GeometryObject {
public:
    static std::unique_ptr<Solid> createFromProfile(const std::vector<Vector3>& baseProfile, float height);
    static std::unique_ptr<Solid> createFromCurve(const Curve& curve, float height);

    ObjectType getType() const override { return ObjectType::Solid; }
    const HalfEdgeMesh& getMesh() const override { return mesh; }
    HalfEdgeMesh& getMesh() override { return mesh; }

    const std::vector<Vector3>& getBaseLoop() const { return baseLoop; }
    float getHeight() const { return height; }

private:
    Solid(std::vector<Vector3> baseLoop, float height, HalfEdgeMesh mesh);

    std::vector<Vector3> baseLoop;
    float height = 0.0f;
    HalfEdgeMesh mesh;
};
