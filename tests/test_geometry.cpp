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
    assert(curveObj->getStableId() != 0);
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
    assert(solidObj->getStableId() != 0);
    assert(solidObj->getStableId() != curveObj->getStableId());
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

    GeometryKernel::ExtrudeOptions options;
    options.capStart = false;
    options.capEnd = true;
    GeometryObject* uncapped = kernel.extrudeCurve(curveObj, height, options);
    assert(uncapped);
    auto* uncappedSolid = static_cast<Solid*>(uncapped);
    const auto& uncappedMesh = uncappedSolid->getMesh();
    bool bottomFound = false;
    for (const auto& face : uncappedMesh.getFaces()) {
        if (face.normal.y < -0.9f)
            bottomFound = true;
    }
    assert(!bottomFound);
    kernel.deleteObject(uncapped);

    Vector3 sweepDirection{ 0.0f, height, 1.5f };
    GeometryObject* swept = kernel.extrudeCurveAlongVector(curveObj, sweepDirection);
    assert(swept);
    auto* sweptSolid = static_cast<Solid*>(swept);
    assert(std::fabs(sweptSolid->getHeight() - sweepDirection.length()) < 1e-5f);
    kernel.deleteObject(swept);

    // tiny extrusion ignored
    assert(kernel.extrudeCurve(curveObj, 1e-5f) == nullptr);

    kernel.assignMaterial(curveObj, "TestMaterial");
    GeometryKernel::ShapeMetadata circleMeta;
    circleMeta.type = GeometryKernel::ShapeMetadata::Type::Circle;
    circleMeta.circle.radius = 1.0f;
    circleMeta.circle.direction = Vector3(0.0f, 1.0f, 0.0f);
    kernel.setShapeMetadata(curveObj, circleMeta);
    assert(kernel.hasShapeMetadata(curveObj->getStableId()));

    GeometryObject::StableId originalCurveId = curveObj->getStableId();
    GeometryObject* clonedCurve = kernel.cloneObject(*curveObj);
    assert(clonedCurve);
    assert(clonedCurve->getStableId() != 0);
    assert(clonedCurve->getStableId() != originalCurveId);
    assert(kernel.getMaterial(clonedCurve) == "TestMaterial");
    auto clonedMetadata = kernel.shapeMetadata(clonedCurve);
    assert(clonedMetadata.has_value());
    assert(clonedMetadata->type == GeometryKernel::ShapeMetadata::Type::Circle);

    const auto& preDeletionMaterials = kernel.getMaterials();
    assert(preDeletionMaterials.count(originalCurveId) == 1);
    assert(preDeletionMaterials.count(clonedCurve->getStableId()) == 1);

    kernel.deleteObject(curveObj);
    const auto& postDeletionMaterials = kernel.getMaterials();
    assert(postDeletionMaterials.count(originalCurveId) == 0);
    assert(postDeletionMaterials.count(clonedCurve->getStableId()) == 1);
    assert(!kernel.hasShapeMetadata(originalCurveId));
    assert(kernel.hasShapeMetadata(clonedCurve->getStableId()));

    GeometryObject::StableId clonedId = clonedCurve->getStableId();
    kernel.deleteObject(clonedCurve);
    const auto& afterCloneRemovalMaterials = kernel.getMaterials();
    assert(afterCloneRemovalMaterials.count(clonedId) == 0);
    assert(!kernel.hasShapeMetadata(clonedId));

    assert(kernel.getObjects().size() == 1);
    assert(kernel.getObjects()[0].get() == solidObj);
    kernel.deleteObject(solidObj);
    assert(kernel.getObjects().empty());

    return 0;
}

