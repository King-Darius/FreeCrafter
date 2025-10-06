#pragma once
#include "Vector3.h"
#include "HalfEdgeMesh.h"

#include <memory>

enum class ObjectType { Curve, Solid };

class GeometryObject {
public:
    virtual ~GeometryObject() = default;
    virtual ObjectType getType() const = 0;
    virtual const HalfEdgeMesh& getMesh() const = 0;
    virtual HalfEdgeMesh& getMesh() = 0;
    virtual std::unique_ptr<GeometryObject> clone() const = 0;
    void setSelected(bool sel) { selected = sel; }
    bool isSelected() const { return selected; }
    void setVisible(bool vis) { visible = vis; }
    bool isVisible() const { return visible; }
    void setHidden(bool hiddenState) { hidden = hiddenState; }
    bool isHidden() const { return hidden; }
private:
    bool selected = false;
    bool visible = true;
    bool hidden = false;
};
