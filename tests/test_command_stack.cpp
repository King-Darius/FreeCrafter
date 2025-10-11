#include <QApplication>
#include <QByteArray>
#include <QUndoStack>

#include <cassert>
#include <cmath>
#include <vector>

#include <QString>

#include "Core/CommandStack.h"
#include "GeometryKernel/Vector3.h"
#include "Scene/Document.h"
#include "Tools/ToolCommands.h"
#include "Tools/ToolGeometryUtils.h"

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    Scene::Document document;
    QUndoStack undoStack;
    Core::CommandStack commandStack(&undoStack);

    Core::CommandContext context;
    context.document = &document;
    context.geometry = &document.geometry();
    context.geometryChanged = [&]() {
        document.synchronizeWithGeometry();
    };
    commandStack.setContext(context);

    // Creation
    std::vector<Vector3> linePoints { Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) };
    commandStack.push(std::make_unique<Tools::CreateCurveCommand>(linePoints, QStringLiteral("Create Line")));
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().size() == 1);

    undoStack.undo();
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().empty());

    undoStack.redo();
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().size() == 1);

    GeometryObject* object = document.geometry().getObjects().front().get();
    assert(object != nullptr);
    Scene::Document::ObjectId objectId = document.objectIdForGeometry(object);
    assert(objectId != 0);

    object->setSelected(true);
    BoundingBox beforeBounds = computeBoundingBox(*object);
    assert(beforeBounds.valid);

    Vector3 translation(1.5f, 0.0f, 0.0f);
    commandStack.push(std::make_unique<Tools::TranslateObjectsCommand>(std::vector<Scene::Document::ObjectId> { objectId }, translation, QStringLiteral("Move Selection")));
    BoundingBox movedBounds = computeBoundingBox(*document.geometry().getObjects().front());
    assert(movedBounds.valid);
    assert(std::fabs((movedBounds.min.x - beforeBounds.min.x) - translation.x) < 1e-4f);

    undoStack.undo();
    BoundingBox restoredBounds = computeBoundingBox(*document.geometry().getObjects().front());
    assert(restoredBounds.valid);
    assert(std::fabs(restoredBounds.min.x - beforeBounds.min.x) < 1e-4f);

    undoStack.redo();
    BoundingBox redoBounds = computeBoundingBox(*document.geometry().getObjects().front());
    assert(redoBounds.valid);
    assert(std::fabs((redoBounds.min.x - beforeBounds.min.x) - translation.x) < 1e-4f);

    GeometryObject* transformed = document.geometry().getObjects().front().get();
    assert(transformed != nullptr);
    Scene::Document::ObjectId transformedId = document.objectIdForGeometry(transformed);
    assert(transformedId != 0);
    transformed->setSelected(true);

    commandStack.push(std::make_unique<Tools::DeleteObjectsCommand>(std::vector<Scene::Document::ObjectId> { transformedId }, QStringLiteral("Delete Selection")));
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().empty());

    undoStack.undo();
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().size() == 1);
    GeometryObject* restored = document.geometry().getObjects().front().get();
    assert(restored != nullptr);
    BoundingBox afterDeletionUndo = computeBoundingBox(*restored);
    assert(afterDeletionUndo.valid);
    assert(std::fabs(afterDeletionUndo.min.x - redoBounds.min.x) < 1e-4f);

    return 0;
}

