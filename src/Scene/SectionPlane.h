#pragma once

#include "../GeometryKernel/Vector3.h"

#include <array>
#include <iosfwd>

namespace Scene {

struct SectionFillStyle {
    float red = 0.93f;
    float green = 0.58f;
    float blue = 0.24f;
    float alpha = 0.55f;
    float extent = 2.5f;
    bool fillEnabled = true;
};

class SectionPlane {
public:
    SectionPlane();

    void setFromOriginAndNormal(const Vector3& origin, const Vector3& normal);
    void setBasis(const Vector3& xAxis, const Vector3& yAxis);
    void setTransform(const std::array<float, 16>& columnMajorMatrix);

    const Vector3& getNormal() const { return planeNormal; }
    float getOffset() const { return planeOffset; }
    const Vector3& getOrigin() const { return planeOrigin; }
    const std::array<float, 16>& getTransform() const { return transformMatrix; }

    SectionFillStyle& fillStyle() { return fill; }
    const SectionFillStyle& fillStyle() const { return fill; }

    bool isActive() const { return active; }
    void setActive(bool value) { active = value; }

    bool isVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }

    void serialize(std::ostream& os) const;
    bool deserialize(std::istream& is);

private:
    void updateOffset();
    void ensureBasis();

    Vector3 planeNormal;
    float planeOffset = 0.0f;
    Vector3 planeOrigin;
    std::array<float, 16> transformMatrix;
    SectionFillStyle fill;
    bool active = true;
    bool visible = true;
};

} // namespace Scene
