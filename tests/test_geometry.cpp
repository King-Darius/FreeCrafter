#include <cassert>
#include <vector>

#include "GeometryKernel.h"
#include "Curve.h"
#include "Solid.h"

int main() {
    GeometryKernel kernel;
    std::vector<Vector3> pts{
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}
    };

    // addCurve
    GeometryObject* curveObj = kernel.addCurve(pts);
    assert(curveObj);
    assert(kernel.getObjects().size() == 1);
    assert(curveObj->getType() == ObjectType::Curve);
    auto* curve = static_cast<Curve*>(curveObj);
    assert(curve->getPoints().size() == pts.size());

    // extrudeCurve
    float height = 2.0f;
    GeometryObject* solidObj = kernel.extrudeCurve(curveObj, height);
    assert(solidObj);
    assert(solidObj->getType() == ObjectType::Solid);
    assert(kernel.getObjects().size() == 2);
    auto* solid = static_cast<Solid*>(solidObj);
    size_t expected = pts.size() + 1; // profile closed by Solid
    assert(solid->getVertices().size() == expected * 2);

    // deleteObject
    kernel.deleteObject(curveObj);
    assert(kernel.getObjects().size() == 1);
    assert(kernel.getObjects()[0].get() == solidObj);
    kernel.deleteObject(solidObj);
    assert(kernel.getObjects().empty());

    return 0;
}

