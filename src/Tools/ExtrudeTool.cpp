#include "ExtrudeTool.h"
#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Curve.h"
#include "../Scene/Document.h"

#include <QObject>
#include <QUndoCommand>
#include <QUndoStack>

namespace {

class ExtrudeCurveCommand : public QUndoCommand {
public:
    ExtrudeCurveCommand(GeometryKernel& kernel, Scene::Document* document, GeometryObject* sourceCurve, float height)
        : geometryKernel(kernel)
        , sceneDocument(document)
        , curve(sourceCurve)
        , extrudeHeight(height)
    {
        setText(QObject::tr("Extrude Curve"));
    }

    void redo() override
    {
        if (!curve)
            return;
        createdSolid = geometryKernel.extrudeCurve(curve, extrudeHeight);
        if (sceneDocument)
            sceneDocument->synchronizeWithGeometry();
    }

    void undo() override
    {
        if (!createdSolid)
            return;
        geometryKernel.deleteObject(createdSolid);
        createdSolid = nullptr;
        if (sceneDocument)
            sceneDocument->synchronizeWithGeometry();
    }

private:
    GeometryKernel& geometryKernel;
    Scene::Document* sceneDocument = nullptr;
    GeometryObject* curve = nullptr;
    float extrudeHeight = 0.0f;
    GeometryObject* createdSolid = nullptr;
};

}

void ExtrudeTool::onPointerDown(const PointerInput&)
{
    if (!geometry)
        return;

    GeometryObject* obj = nullptr;
    const auto& objs = geometry->getObjects();
    for (auto it = objs.rbegin(); it != objs.rend(); ++it) {
        if ((*it)->getType() == ObjectType::Curve) {
            obj = it->get();
            break;
        }
    }
    if (!obj)
        return;

    constexpr float kDefaultHeight = 1.0f;
    if (auto* stack = getUndoStack()) {
        stack->push(new ExtrudeCurveCommand(*geometry, getDocument(), obj, kDefaultHeight));
    } else {
        geometry->extrudeCurve(obj, kDefaultHeight);
        if (auto* doc = getDocument())
            doc->synchronizeWithGeometry();
    }
}
