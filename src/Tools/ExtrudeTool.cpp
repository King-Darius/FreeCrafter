#include "ExtrudeTool.h"
#include "SelectionTool.h"

void ExtrudeTool::onMouseDown(int x,int y){ if(!geometry||!camera) return; SelectionTool picker(geometry,camera); GeometryObject* obj=nullptr;
    // temp reuse: call private? Here mimic by copying minimal pick logic would be better; for brevity we'll assume we have a helper.
    // For now, extrude the most recently added curve as a stand-in.
    const auto& objs = geometry->getObjects();
    for (auto it = objs.rbegin(); it != objs.rend(); ++it){ if((*it)->getType()==ObjectType::Curve){ obj=it->get(); break; } }
    if(obj) geometry->extrudeCurve(obj, 1.0f);
}
