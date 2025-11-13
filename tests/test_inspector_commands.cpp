#include <QApplication>
#include <QByteArray>
#include <QUndoStack>

#include <cassert>
#include <cmath>
#include <vector>

#include <QString>

#include "Core/CommandStack.h"
#include "GeometryKernel/ShapeBuilder.h"
#include "Scene/Document.h"
#include "Scene/SceneGraphCommands.h"
#include "Tools/ToolCommands.h"

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

    GeometryKernel::ShapeMetadata circleMeta;
    circleMeta.type = GeometryKernel::ShapeMetadata::Type::Circle;
    circleMeta.circle.center = Vector3(0.0f, 0.0f, 0.0f);
    circleMeta.circle.radius = 2.0f;
    circleMeta.circle.segments = 32;

    auto circlePoints = ShapeBuilder::buildCircle(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), circleMeta.circle.segments);
    commandStack.push(std::make_unique<Tools::CreateCurveCommand>(circlePoints, QStringLiteral("Create Circle"), circleMeta, "Circle A"));
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().size() == 1);

    GeometryObject* circleObject = document.geometry().getObjects().front().get();
    assert(circleObject != nullptr);
    Scene::Document::ObjectId circleId = document.objectIdForGeometry(circleObject);
    assert(circleId != 0);

    GeometryKernel::ShapeMetadata modifiedMeta = circleMeta;
    modifiedMeta.circle.radius = 4.0f;
    modifiedMeta.circle.segments = 48;
    commandStack.push(std::make_unique<Scene::RebuildCurveFromMetadataCommand>(circleId, modifiedMeta, QStringLiteral("Update Circle")));
    auto updatedMeta = document.geometry().shapeMetadata(circleObject);
    assert(updatedMeta.has_value());
    assert(std::fabs(updatedMeta->circle.radius - 4.0f) < 1e-4f);
    assert(updatedMeta->circle.segments == 48);
    undoStack.undo();
    updatedMeta = document.geometry().shapeMetadata(circleObject);
    assert(updatedMeta.has_value());
    assert(std::fabs(updatedMeta->circle.radius - circleMeta.circle.radius) < 1e-4f);
    undoStack.redo();

    // Duplicate another curve so bulk commands can be validated.
    commandStack.push(std::make_unique<Tools::CreateCurveCommand>(circlePoints, QStringLiteral("Create Circle"), circleMeta, "Circle B"));
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().size() == 2);
    GeometryObject* secondObject = document.geometry().getObjects().back().get();
    Scene::Document::ObjectId secondId = document.objectIdForGeometry(secondObject);
    assert(secondId != 0);

    std::vector<Scene::Document::ObjectId> ids { circleId, secondId };
    commandStack.push(std::make_unique<Scene::RenameObjectsCommand>(ids, QStringLiteral("Renamed")));
    const Scene::Document::ObjectNode* firstNode = document.findObject(circleId);
    const Scene::Document::ObjectNode* secondNode = document.findObject(secondId);
    assert(firstNode && secondNode);
    assert(firstNode->name == "Renamed");
    assert(secondNode->name == "Renamed");
    undoStack.undo();
    firstNode = document.findObject(circleId);
    secondNode = document.findObject(secondId);
    assert(firstNode && secondNode);
    assert(firstNode->name == "Circle A");
    assert(secondNode->name == "Circle B");
    undoStack.redo();

    commandStack.push(std::make_unique<Scene::SetObjectsVisibilityCommand>(ids, false));
    firstNode = document.findObject(circleId);
    secondNode = document.findObject(secondId);
    assert(firstNode && !firstNode->visible);
    assert(secondNode && !secondNode->visible);
    undoStack.undo();
    firstNode = document.findObject(circleId);
    secondNode = document.findObject(secondId);
    assert(firstNode && firstNode->visible);
    assert(secondNode && secondNode->visible);
    undoStack.redo();

    Scene::SceneSettings::Color tagColor { 0.4f, 0.2f, 0.8f, 1.0f };
    commandStack.push(std::make_unique<Scene::CreateTagCommand>(QStringLiteral("Selection"), tagColor));
    Scene::Document::TagId tagId = 0;
    for (const auto& entry : document.tags()) {
        if (entry.second.name == "Selection") {
            tagId = entry.first;
            break;
        }
    }
    assert(tagId != 0);

    commandStack.push(std::make_unique<Scene::SetTagAssignmentsCommand>(tagId, ids, true));
    firstNode = document.findObject(circleId);
    secondNode = document.findObject(secondId);
    assert(firstNode && std::find(firstNode->tags.begin(), firstNode->tags.end(), tagId) != firstNode->tags.end());
    assert(secondNode && std::find(secondNode->tags.begin(), secondNode->tags.end(), tagId) != secondNode->tags.end());
    undoStack.undo();
    firstNode = document.findObject(circleId);
    secondNode = document.findObject(secondId);
    assert(firstNode && std::find(firstNode->tags.begin(), firstNode->tags.end(), tagId) == firstNode->tags.end());
    assert(secondNode && std::find(secondNode->tags.begin(), secondNode->tags.end(), tagId) == secondNode->tags.end());
    undoStack.redo();

    return 0;
}

