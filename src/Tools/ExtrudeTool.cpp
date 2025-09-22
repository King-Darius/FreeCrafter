#include "ExtrudeTool.h"
#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Curve.h"

#include <cmath>

void ExtrudeTool::onMouseDown(int,int){
    // extrude the most recently added curve by configured height
    GeometryObject* obj = nullptr;
    const auto& objs = geometry->getObjects();
    for (auto it = objs.rbegin(); it != objs.rend(); ++it){
        if ((*it)->getType()==ObjectType::Curve){ obj = it->get(); break; }
    }
    if (obj) {
        geometry->extrudeCurve(obj, static_cast<float>(pendingDistance));
    }
}

bool ExtrudeTool::applyNumericInput(double value)
{
    if (std::isfinite(value)) {
        pendingDistance = value;
        return true;
    }
    return false;
}
