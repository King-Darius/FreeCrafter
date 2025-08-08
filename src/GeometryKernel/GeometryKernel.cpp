#include "GeometryKernel.h"

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
