#include "GeometryKernel.h"
#include "Serialization.h"
#include <fstream>
#include <string>

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

void GeometryKernel::deleteObject(GeometryObject* obj) {
    if (!obj) return;
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->get() == obj) { objects.erase(it); return; }
    }
}

void GeometryKernel::clear() { objects.clear(); }

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
    objects.clear();
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
    objects.clear();
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
