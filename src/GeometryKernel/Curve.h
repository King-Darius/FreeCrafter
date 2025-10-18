#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "GeometryObject.h"

class Curve : public GeometryObject {
public:
    static std::unique_ptr<Curve> createFromPoints(const std::vector<Vector3>& pts,
        const std::vector<bool>& edgeHardness = {});
    static std::unique_ptr<Curve> createOpenPolyline(const std::vector<Vector3>& pts,
        const std::vector<bool>& edgeHardness = {});
    bool rebuildFromPoints(const std::vector<Vector3>& pts, const std::vector<bool>& edgeHardness = {});

    ObjectType getType() const override { return ObjectType::Curve; }
    const HalfEdgeMesh& getMesh() const override { return mesh; }
    HalfEdgeMesh& getMesh() override { return mesh; }
    std::unique_ptr<GeometryObject> clone() const override;

    const std::vector<Vector3>& getBoundaryLoop() const { return boundaryLoop; }
    const std::vector<bool>& getEdgeHardness() const { return hardnessFlags; }
    void setEdgeHardness(std::vector<bool> hardness);
    void tagAllEdgesHard(bool hard);

    void applyTransform(const std::function<Vector3(const Vector3&)>& fn);
    void translate(const Vector3& delta);
    void rotate(const Vector3& pivot, const Vector3& axis, float angleRadians);
    void scale(const Vector3& pivot, const Vector3& factors);

private:
    Curve(std::vector<Vector3> loop, HalfEdgeMesh mesh, std::vector<bool> hardness);
    static std::unique_ptr<Curve> makePolyline(std::vector<Vector3>&& sanitized,
        const std::vector<bool>& hardnessOverride);

    std::vector<Vector3> boundaryLoop;
    HalfEdgeMesh mesh;
    std::vector<bool> hardnessFlags;
};
