#pragma once

#include "../Scene/Document.h"

#include <QUndoCommand>

#include <functional>
#include <vector>

class GeometryKernel;

namespace Core {

struct CommandContext {
    Scene::Document* document = nullptr;
    GeometryKernel* geometry = nullptr;
    std::function<void()> geometryChanged;
    std::function<void(const std::vector<Scene::Document::ObjectId>&)> selectionChanged;
};

class Command : public QUndoCommand {
public:
    explicit Command(const QString& text = QString());

    void setContext(const CommandContext& context);

    void redo() override final;
    void undo() override final;

protected:
    virtual void initialize();
    virtual void performRedo() = 0;
    virtual void performUndo() = 0;

    Scene::Document* document() const { return context.document; }
    GeometryKernel* geometry() const { return context.geometry; }

    void setFinalSelection(const std::vector<Scene::Document::ObjectId>& ids);
    void setFinalSelection(std::vector<Scene::Document::ObjectId>&& ids);
    void overrideInitialSelection(const std::vector<Scene::Document::ObjectId>& ids);
    const std::vector<Scene::Document::ObjectId>& initialSelection() const { return beforeSelection; }

    std::vector<Scene::Document::ObjectId> currentSelection() const;
    void applySelection(const std::vector<Scene::Document::ObjectId>& ids) const;

    void notifyGeometryChanged() const;

private:
    CommandContext context;
    std::vector<Scene::Document::ObjectId> beforeSelection;
    std::vector<Scene::Document::ObjectId> afterSelection;
    bool initialized = false;
};

} // namespace Core

