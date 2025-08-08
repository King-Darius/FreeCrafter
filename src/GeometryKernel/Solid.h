#pragma once
#include <vector>
#include "GeometryObject.h"
#include "Vector3.h"

class Solid : public GeometryObject {
public:
    struct Face { std::vector<int> indices; };
    Solid(const std::vector<Vector3>& baseProfile, float height);
    ObjectType getType() const override { return ObjectType::Solid; }
    const std::vector<Vector3>& getVertices() const { return vertices; }
    const std::vector<Face>& getFaces() const { return faces; }
private:
    std::vector<Vector3> vertices;
    std::vector<Face> faces;
};
