#pragma once

#include <QColor>
#include <QWidget>

#include "../Scene/Document.h"

class QUndoStack;
class InspectorPanel;
class OutlinerPanel;
class HistoryPanel;
class EnvironmentPanel;
class MaterialsPanel;
class TagsPanel;

namespace Core {
class CommandStack;
}

class GeometryKernel;

class RightTray : public QWidget {
    Q_OBJECT
public:
    explicit RightTray(Scene::Document* document,
                      GeometryKernel* geometry,
                      Core::CommandStack* commandStack,
                      QUndoStack* undoStack,
                      QWidget* parent = nullptr);

    InspectorPanel* inspectorPanel() const { return inspector_; }
    OutlinerPanel* outlinerPanel() const { return outliner_; }
    HistoryPanel* historyPanel() const { return history_; }
    EnvironmentPanel* environmentPanel() const { return environment_; }

    MaterialsPanel* materialsPanel() const { return materials_; }
    TagsPanel* tagsPanel() const { return tags_; }

    void refreshPanels();

private:
    void handleRenameObject(Scene::Document::ObjectId id, const QString& name);
    void handleVisibilityChange(Scene::Document::ObjectId id, bool visible);
    void handleExpansionChange(Scene::Document::ObjectId id, bool expanded);
    void handleAssignMaterial(const QString& name);
    void handleCreateTag(const QString& name, const QColor& color);
    void handleRenameTag(Scene::Document::TagId id, const QString& name);
    void handleSetTagVisibility(Scene::Document::TagId id, bool visible);
    void handleSetTagColor(Scene::Document::TagId id, const QColor& color);

    Scene::Document* document_ = nullptr;
    GeometryKernel* geometry_ = nullptr;
    Core::CommandStack* commandStack_ = nullptr;
    InspectorPanel* inspector_ = nullptr;
    OutlinerPanel* outliner_ = nullptr;
    HistoryPanel* history_ = nullptr;
    EnvironmentPanel* environment_ = nullptr;
    MaterialsPanel* materials_ = nullptr;
    TagsPanel* tags_ = nullptr;
};
