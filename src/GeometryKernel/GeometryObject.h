#pragma once
#include "Vector3.h"

enum class ObjectType { Curve, Solid };

class GeometryObject {
public:
    virtual ~GeometryObject() = default;
    virtual ObjectType getType() const = 0;
    void setSelected(bool sel) { selected = sel; }
    bool isSelected() const { return selected; }
private:
    bool selected = false;
};
