#pragma once
#include <vector>
#include "GeometryObject.h"

class Curve : public GeometryObject {
public:
    explicit Curve(const std::vector<Vector3>& pts);
    ObjectType getType() const override { return ObjectType::Curve; }
    const std::vector<Vector3>& getPoints() const { return points; }
private:
    std::vector<Vector3> points;
};
