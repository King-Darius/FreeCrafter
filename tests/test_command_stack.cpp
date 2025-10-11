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
#include "Scene/SceneGraphCommands.h"
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

    // Scene graph commands
    Scene::Document::ObjectId renameId = document.objectIdForGeometry(restored);
    commandStack.push(std::make_unique<Scene::RenameObjectCommand>(renameId, QStringLiteral("Renamed")));
    const Scene::Document::ObjectNode* renamedNode = document.findObject(renameId);
    assert(renamedNode && renamedNode->name == "Renamed");
    undoStack.undo();
    renamedNode = document.findObject(renameId);
    assert(renamedNode && renamedNode->name != "Renamed");
    undoStack.redo();
    renamedNode = document.findObject(renameId);
    assert(renamedNode && renamedNode->name == "Renamed");

    commandStack.push(std::make_unique<Scene::AssignMaterialCommand>(std::vector<Scene::Document::ObjectId> { renameId }, QStringLiteral("Brick")));
    assert(document.geometry().getMaterial(restored) == "Brick");
    undoStack.undo();
    assert(document.geometry().getMaterial(restored).empty());
    undoStack.redo();
    assert(document.geometry().getMaterial(restored) == "Brick");

    Scene::SceneSettings::Color tagColor { 0.8f, 0.1f, 0.1f, 1.0f };
    commandStack.push(std::make_unique<Scene::CreateTagCommand>(QStringLiteral("Exterior"), tagColor));
    assert(!document.tags().empty());
    Scene::Document::TagId createdTag = 0;
    for (const auto& entry : document.tags()) {
        if (entry.second.name == "Exterior") {
            createdTag = entry.first;
            break;
        }
    }
    assert(createdTag != 0);

    commandStack.push(std::make_unique<Scene::RenameTagCommand>(createdTag, QStringLiteral("Facade")));
    assert(document.tags().at(createdTag).name == "Facade");
    undoStack.undo();
    assert(document.tags().at(createdTag).name == "Exterior");
    undoStack.redo();
    assert(document.tags().at(createdTag).name == "Facade");

    commandStack.push(std::make_unique<Scene::SetTagVisibilityCommand>(createdTag, false));
    assert(!document.tags().at(createdTag).visible);
    undoStack.undo();
    assert(document.tags().at(createdTag).visible);
    undoStack.redo();
    assert(!document.tags().at(createdTag).visible);

    Scene::SceneSettings::Color newColor { 0.2f, 0.4f, 0.6f, 1.0f };
    commandStack.push(std::make_unique<Scene::SetTagColorCommand>(createdTag, newColor));
    auto currentColor = document.tags().at(createdTag).color;
    assert(std::fabs(currentColor.r - newColor.r) < 1e-5f);
    undoStack.undo();
    currentColor = document.tags().at(createdTag).color;
    assert(std::fabs(currentColor.r - tagColor.r) < 1e-5f);
    undoStack.redo();
    currentColor = document.tags().at(createdTag).color;
    assert(std::fabs(currentColor.r - newColor.r) < 1e-5f);

    return 0;
}

