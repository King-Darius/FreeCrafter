#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "GeometryObject.h"
#include "Vector3.h"

class Curve;

class Solid : public GeometryObject {
public:
    static std::unique_ptr<Solid> createFromProfile(const std::vector<Vector3>& baseProfile, float height);
    static std::unique_ptr<Solid> createFromCurve(const Curve& curve, float height);
    static std::unique_ptr<Solid> createFromMesh(HalfEdgeMesh mesh);

    ObjectType getType() const override { return ObjectType::Solid; }
    const HalfEdgeMesh& getMesh() const override { return mesh; }
    HalfEdgeMesh& getMesh() override { return mesh; }
    std::unique_ptr<GeometryObject> clone() const override;

    const std::vector<Vector3>& getBaseLoop() const { return baseLoop; }
    float getHeight() const { return height; }

    void applyTransform(const std::function<Vector3(const Vector3&)>& fn);
    void translate(const Vector3& delta);
    void rotate(const Vector3& pivot, const Vector3& axis, float angleRadians);
    void scale(const Vector3& pivot, const Vector3& factors);
    void setMesh(HalfEdgeMesh meshData);
    void setBaseMetadata(std::vector<Vector3> base, float newHeight);

private:
    Solid(std::vector<Vector3> baseLoop, float height, HalfEdgeMesh mesh);

    std::vector<Vector3> baseLoop;
    float height = 0.0f;
    HalfEdgeMesh mesh;
};
