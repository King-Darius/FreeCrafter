#pragma once
#include <memory>
#include <vector>
#include "GeometryObject.h"

class Curve : public GeometryObject {
public:
    static std::unique_ptr<Curve> createFromPoints(const std::vector<Vector3>& pts);

    ObjectType getType() const override { return ObjectType::Curve; }
    const HalfEdgeMesh& getMesh() const override { return mesh; }
    HalfEdgeMesh& getMesh() override { return mesh; }

    const std::vector<Vector3>& getBoundaryLoop() const { return boundaryLoop; }

private:
    Curve(std::vector<Vector3> loop, HalfEdgeMesh mesh);

    std::vector<Vector3> boundaryLoop;
    HalfEdgeMesh mesh;
};
