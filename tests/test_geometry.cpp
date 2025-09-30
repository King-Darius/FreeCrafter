#include <cassert>
#include <cmath>
#include <vector>

#include "GeometryKernel.h"
#include "Curve.h"
#include "Solid.h"

int main() {
    GeometryKernel kernel;

    // invalid curve rejected
    assert(kernel.addCurve({}) == nullptr);

    std::vector<Vector3> pts{
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}, // duplicate
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.00001f}, // tiny edge
        {1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 0.0f} // closing duplicate
    };

    GeometryObject* curveObj = kernel.addCurve(pts);
    assert(curveObj);
    assert(kernel.getObjects().size() == 1);
    assert(curveObj->getType() == ObjectType::Curve);
    auto* curve = static_cast<Curve*>(curveObj);
    const auto& boundary = curve->getBoundaryLoop();
    assert(boundary.size() == 4); // duplicates and tiny edges collapsed

    const auto& curveMesh = curve->getMesh();
    assert(curveMesh.isManifold());
    assert(curveMesh.getFaces().size() == 1);
    assert(curveMesh.getTriangles().size() == 2);
    assert(curveMesh.getFaces()[0].normal.y > 0.9f);

    float height = 2.0f;
    GeometryObject* solidObj = kernel.extrudeCurve(curveObj, height);
    assert(solidObj);
    assert(solidObj->getType() == ObjectType::Solid);
    assert(kernel.getObjects().size() == 2);
    auto* solid = static_cast<Solid*>(solidObj);
    assert(std::fabs(solid->getHeight() - height) < 1e-6f);
    assert(solid->getBaseLoop().size() == boundary.size());

    const auto& solidMesh = solid->getMesh();
    assert(solidMesh.isManifold());
    size_t loopSize = boundary.size();
    assert(solidMesh.getVertices().size() == loopSize * 2);
    assert(solidMesh.getFaces().size() == loopSize + 2);
    assert(solidMesh.getTriangles().size() == 4 * loopSize - 4);

    bool foundTop = false;
    bool foundBottom = false;
    for (const auto& face : solidMesh.getFaces()) {
        if (face.normal.y > 0.9f) foundTop = true;
        if (face.normal.y < -0.9f) foundBottom = true;
    }
    assert(foundTop && foundBottom);

    // tiny extrusion ignored
    assert(kernel.extrudeCurve(curveObj, 1e-5f) == nullptr);

    kernel.deleteObject(curveObj);
    assert(kernel.getObjects().size() == 1);
    assert(kernel.getObjects()[0].get() == solidObj);
    kernel.deleteObject(solidObj);
    assert(kernel.getObjects().empty());

    return 0;
}

