#include "ExtrudeTool.h"
#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Curve.h"

void ExtrudeTool::onMouseDown(int,int){
    // extrude the most recently added curve by unit height
    GeometryObject* obj = nullptr;
    const auto& objs = geometry->getObjects();
    for (auto it = objs.rbegin(); it != objs.rend(); ++it){
        if ((*it)->getType()==ObjectType::Curve){ obj = it->get(); break; }
    }
    if (obj) geometry->extrudeCurve(obj, 1.0f);
}
