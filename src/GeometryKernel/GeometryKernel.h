#pragma once
#include <vector>
#include <memory>
#include <string>
#include <iosfwd>
#include "GeometryObject.h"
#include "Curve.h"
#include "Solid.h"

class GeometryKernel {
public:
    GeometryKernel() = default;
    GeometryObject* addCurve(const std::vector<Vector3>& points);
    GeometryObject* extrudeCurve(GeometryObject* curveObj, float height);
    void deleteObject(GeometryObject* obj);
    void clear();
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    void saveToStream(std::ostream& os) const;
    void loadFromStream(std::istream& is, const std::string& terminator);
    const std::vector<std::unique_ptr<GeometryObject>>& getObjects() const { return objects; }
private:
    std::vector<std::unique_ptr<GeometryObject>> objects;
};
