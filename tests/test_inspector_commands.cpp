#include <cassert>
#include <cmath>
#include <vector>

#include <QCoreApplication>
#include <QString>
#include <QUndoStack>

#include "Scene/Document.h"
#include "Scene/SceneGraphCommands.h"
#include "Core/CommandStack.h"
#include "GeometryKernel/Vector3.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    Scene::Document document;
    GeometryKernel& geometry = document.geometry();

    std::vector<Vector3> points { Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) };
    GeometryObject* curve = geometry.addCurve(points);
    assert(curve != nullptr);

    Scene::Document::ObjectId objectId = document.ensureObjectForGeometry(curve, "Edge");
    assert(objectId != 0);

    QUndoStack undoStack;
    Core::CommandStack commandStack(&undoStack);
    Core::CommandContext context;
    context.document = &document;
    context.geometry = &geometry;
    commandStack.setContext(context);

    // Rename command
    {
        auto rename = std::make_unique<Scene::RenameObjectCommand>(objectId, QStringLiteral("Renamed"));
        commandStack.push(std::move(rename));
        const auto* node = document.findObject(objectId);
        assert(node != nullptr);
        assert(node->name == "Renamed");

        undoStack.undo();
        node = document.findObject(objectId);
        assert(node != nullptr);
        assert(node->name == "Edge");

        undoStack.redo();
        node = document.findObject(objectId);
        assert(node != nullptr);
        assert(node->name == "Renamed");
    }

    // Visibility toggle
    {
        auto hide = std::make_unique<Scene::SetObjectVisibilityCommand>(objectId, false);
        commandStack.push(std::move(hide));
        const auto* node = document.findObject(objectId);
        assert(node != nullptr);
        assert(!node->visible);

        undoStack.undo();
        node = document.findObject(objectId);
        assert(node != nullptr);
        assert(node->visible);
    }

    // Transform command (translation along X)
    {
        Scene::Document::Transform before = document.objectTransform(objectId);
        Scene::Document::Transform after = before;
        after.position.x = before.position.x + 1.0f;
        Scene::Document::TransformMask mask;
        mask.position[0] = true;

        std::vector<Scene::SetObjectTransformCommand::TransformChange> changes;
        changes.push_back({ objectId, before, after, mask });

        auto move = std::make_unique<Scene::SetObjectTransformCommand>(std::move(changes), QStringLiteral("Move"));
        commandStack.push(std::move(move));

        Scene::Document::Transform current = document.objectTransform(objectId);
        assert(std::fabs(current.position.x - (before.position.x + 1.0f)) < 1e-4f);

        undoStack.undo();
        current = document.objectTransform(objectId);
        assert(std::fabs(current.position.x - before.position.x) < 1e-4f);

        undoStack.redo();
        current = document.objectTransform(objectId);
        assert(std::fabs(current.position.x - (before.position.x + 1.0f)) < 1e-4f);
    }

    return 0;
}

