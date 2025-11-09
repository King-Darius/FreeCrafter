#include "RightTray.h"

#include "CollapsibleSection.h"
#include "EnvironmentPanel.h"
#include "HistoryPanel.h"
#include "InspectorPanel.h"
#include "MaterialsPanel.h"
#include "OutlinerPanel.h"
#include "TagsPanel.h"

#include "GeometryKernel/GeometryKernel.h"
#include "Scene/SceneGraphCommands.h"

#include "../Core/CommandStack.h"

#include <QColor>
#include <vector>
#include <memory>

#include <QScrollArea>
#include <QVBoxLayout>
#include <QUndoStack>
#include <QObject>

RightTray::RightTray(Scene::Document* document,
                     GeometryKernel* geometry,
                     Core::CommandStack* commandStack,
                     QUndoStack* undoStack,
                     QWidget* parent)
    : QWidget(parent)
    , document_(document)
    , geometry_(geometry)
    , commandStack_(commandStack)
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);

    auto* container = new QWidget;
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);

    inspector_ = new InspectorPanel(container);
    outliner_ = new OutlinerPanel(container);
    undoStack_ = undoStack;
    history_ = new HistoryPanel(undoStack_, container);
    environment_ = new EnvironmentPanel(container);
    materials_ = new MaterialsPanel(container);
    tags_ = new TagsPanel(container);

    outliner_->setDocument(document_);
    materials_->setDocument(document_);
    tags_->setDocument(document_);

    connect(outliner_, &OutlinerPanel::requestRename, this, &RightTray::handleRenameObject);
    connect(outliner_, &OutlinerPanel::requestVisibilityChange, this, &RightTray::handleVisibilityChange);
    connect(outliner_, &OutlinerPanel::requestExpansionChange, this, &RightTray::handleExpansionChange);

    connect(materials_, &MaterialsPanel::assignMaterialRequested, this, &RightTray::handleAssignMaterial);

    connect(tags_, &TagsPanel::requestCreateTag, this, &RightTray::handleCreateTag);
    connect(tags_, &TagsPanel::requestRenameTag, this, &RightTray::handleRenameTag);
    connect(tags_, &TagsPanel::requestSetTagVisibility, this, &RightTray::handleSetTagVisibility);
    connect(tags_, &TagsPanel::requestSetTagColor, this, &RightTray::handleSetTagColor);

    layout->addWidget(new CollapsibleSection(tr("Inspector"), inspector_, true));
    layout->addWidget(new CollapsibleSection(tr("Outliner"), outliner_, true));
    layout->addWidget(new CollapsibleSection(tr("Materials"), materials_, true));
    layout->addWidget(new CollapsibleSection(tr("Tags"), tags_, true));
    layout->addWidget(new CollapsibleSection(tr("History"), history_, true));
    layout->addWidget(new CollapsibleSection(tr("Environment"), environment_, true));
    layout->addStretch();

    scroll->setWidget(container);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    if (undoStack_) {
        // Keep the History pane in sync with programmatic clears (e.g. new/open)
        // as well as with regular undo/redo interactions.
        connect(undoStack_, &QUndoStack::indexChanged, this, &RightTray::refreshHistory);
        connect(undoStack_, &QUndoStack::cleanChanged, this, &RightTray::refreshHistory);
        connect(undoStack_, &QObject::destroyed, this, [this]() {
            undoStack_ = nullptr;
            refreshHistory();
        });
    }
}

void RightTray::refreshPanels()
{
    if (outliner_)
        outliner_->refresh();
    if (materials_)
        materials_->refresh();
    if (tags_)
        tags_->refresh();
    refreshHistory();
}

void RightTray::refreshHistory()
{
    if (history_)
        history_->refresh();
}

void RightTray::handleRenameObject(Scene::Document::ObjectId id, const QString& name)
{
    if (!commandStack_ || !document_)
        return;
    auto command = std::make_unique<Scene::RenameObjectCommand>(id, name);
    commandStack_->push(std::move(command));
    refreshPanels();
}

void RightTray::handleVisibilityChange(Scene::Document::ObjectId id, bool visible)
{
    if (!commandStack_ || !document_)
        return;
    auto command = std::make_unique<Scene::SetObjectVisibilityCommand>(id, visible);
    commandStack_->push(std::move(command));
    refreshPanels();
}

void RightTray::handleExpansionChange(Scene::Document::ObjectId id, bool expanded)
{
    if (!document_)
        return;
    document_->setObjectExpanded(id, expanded);
}

void RightTray::handleAssignMaterial(const QString& name)
{
    if (!commandStack_ || !document_ || !geometry_)
        return;

    std::vector<Scene::Document::ObjectId> selection;
    for (const auto& object : geometry_->getObjects()) {
        if (!object->isSelected())
            continue;
        Scene::Document::ObjectId id = document_->objectIdForGeometry(object.get());
        if (id != 0)
            selection.push_back(id);
    }
    if (selection.empty())
        return;

    auto command = std::make_unique<Scene::AssignMaterialCommand>(selection, name);
    commandStack_->push(std::move(command));
    refreshPanels();
}

void RightTray::handleCreateTag(const QString& name, const QColor& color)
{
    if (!commandStack_)
        return;
    Scene::SceneSettings::Color tagColor { color.redF(), color.greenF(), color.blueF(), 1.0f };
    auto command = std::make_unique<Scene::CreateTagCommand>(name, tagColor);
    commandStack_->push(std::move(command));
    refreshPanels();
}

void RightTray::handleRenameTag(Scene::Document::TagId id, const QString& name)
{
    if (!commandStack_)
        return;
    auto command = std::make_unique<Scene::RenameTagCommand>(id, name);
    commandStack_->push(std::move(command));
    refreshPanels();
}

void RightTray::handleSetTagVisibility(Scene::Document::TagId id, bool visible)
{
    if (!commandStack_)
        return;
    auto command = std::make_unique<Scene::SetTagVisibilityCommand>(id, visible);
    commandStack_->push(std::move(command));
    refreshPanels();
}

void RightTray::handleSetTagColor(Scene::Document::TagId id, const QColor& color)
{
    if (!commandStack_)
        return;
    Scene::SceneSettings::Color tagColor { color.redF(), color.greenF(), color.blueF(), 1.0f };
    auto command = std::make_unique<Scene::SetTagColorCommand>(id, tagColor);
    commandStack_->push(std::move(command));
    refreshPanels();
}
