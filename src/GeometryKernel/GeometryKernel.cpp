#include "GeometryKernel.h"
#include "Serialization.h"
#include <fstream>

GeometryObject* GeometryKernel::addCurve(const std::vector<Vector3>& points) {
    auto obj = std::make_unique<Curve>(points);
    GeometryObject* raw = obj.get();
    objects.push_back(std::move(obj));
    return raw;
}

GeometryObject* GeometryKernel::extrudeCurve(GeometryObject* curveObj, float height) {
    if (!curveObj || curveObj->getType() != ObjectType::Curve) return nullptr;
    const auto& pts = static_cast<Curve*>(curveObj)->getPoints();
    if (pts.empty()) return nullptr;
    auto obj = std::make_unique<Solid>(pts, height);
    GeometryObject* raw = obj.get();
    objects.push_back(std::move(obj));
    return raw;
}

void GeometryKernel::deleteObject(GeometryObject* obj) {
    if (!obj) return;
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->get() == obj) { objects.erase(it); return; }
    }
}

void GeometryKernel::clear() {
    objects.clear();
}

bool GeometryKernel::saveToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return false;
    out << objects.size() << "\n";
    for (const auto& obj : objects) {
        if (obj->getType() == ObjectType::Curve) {
            GeometryIO::writeCurve(out, *static_cast<Curve*>(obj.get()));
        } else if (obj->getType() == ObjectType::Solid) {
            GeometryIO::writeSolid(out, *static_cast<Solid*>(obj.get()));
        }
    }
    return true;
}

bool GeometryKernel::loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return false;
    clear();
    size_t count;
    if (!(in >> count)) return false;
    for (size_t i = 0; i < count; ++i) {
        std::string type;
        if (!(in >> type)) break;
        if (type == "Curve") {
            auto curve = GeometryIO::readCurve(in);
            if (curve) objects.push_back(std::move(curve));
        } else if (type == "Solid") {
            auto solid = GeometryIO::readSolid(in);
            if (solid) objects.push_back(std::move(solid));
        }
    }
    return true;
}

