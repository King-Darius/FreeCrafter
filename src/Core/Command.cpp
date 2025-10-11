#include "Command.h"

#include "GeometryKernel/GeometryKernel.h"

namespace Core {

Command::Command(const QString& text)
    : QUndoCommand(text)
{
}

void Command::setContext(const CommandContext& ctx)
{
    context = ctx;
}

void Command::redo()
{
    if (!geometry())
        return;
    if (!initialized) {
        beforeSelection = currentSelection();
        initialize();
        initialized = true;
    }

    afterSelection.clear();
    performRedo();
    if (afterSelection.empty())
        afterSelection = beforeSelection;

    applySelection(afterSelection);
    if (context.selectionChanged)
        context.selectionChanged(afterSelection);
    notifyGeometryChanged();
}

void Command::undo()
{
    performUndo();
    applySelection(beforeSelection);
    if (context.selectionChanged)
        context.selectionChanged(beforeSelection);
    notifyGeometryChanged();
}

void Command::initialize()
{
}

void Command::setFinalSelection(const std::vector<Scene::Document::ObjectId>& ids)
{
    afterSelection = ids;
}

void Command::setFinalSelection(std::vector<Scene::Document::ObjectId>&& ids)
{
    afterSelection = std::move(ids);
}

void Command::overrideInitialSelection(const std::vector<Scene::Document::ObjectId>& ids)
{
    beforeSelection = ids;
}

std::vector<Scene::Document::ObjectId> Command::currentSelection() const
{
    std::vector<Scene::Document::ObjectId> result;
    if (!geometry() || !document())
        return result;

    for (const auto& object : geometry()->getObjects()) {
        if (!object->isSelected())
            continue;
        Scene::Document::ObjectId id = document()->objectIdForGeometry(object.get());
        if (id != 0)
            result.push_back(id);
    }
    return result;
}

void Command::applySelection(const std::vector<Scene::Document::ObjectId>& ids) const
{
    if (!geometry() || !document())
        return;

    for (const auto& object : geometry()->getObjects()) {
        object->setSelected(false);
    }

    for (Scene::Document::ObjectId id : ids) {
        if (GeometryObject* object = document()->geometryForObject(id)) {
            object->setSelected(true);
        }
    }
}

void Command::notifyGeometryChanged() const
{
    if (context.geometryChanged)
        context.geometryChanged();
}

} // namespace Core

