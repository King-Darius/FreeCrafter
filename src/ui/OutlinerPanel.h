#pragma once

#include <QTreeWidget>
#include <QWidget>

#include <vector>

#include "Scene/Document.h"

namespace Core {
class CommandStack;
}

class OutlinerPanel : public QWidget {
    Q_OBJECT
public:
    explicit OutlinerPanel(QWidget* parent = nullptr);

    void setDocument(Scene::Document* document);
    void refresh();

    std::vector<Scene::Document::ObjectId> selectedObjectIds() const;

signals:
    void requestRename(Scene::Document::ObjectId id, const QString& name);
    void requestVisibilityChange(Scene::Document::ObjectId id, bool visible);
    void requestExpansionChange(Scene::Document::ObjectId id, bool expanded);

public slots:
    void setCommandStack(Core::CommandStack* stack);

private slots:
    void handleItemChanged(QTreeWidgetItem* item, int column);
    void handleItemExpanded(QTreeWidgetItem* item);
    void handleItemCollapsed(QTreeWidgetItem* item);

private:
    void rebuildTree();
    void populateNode(QTreeWidgetItem* parentItem, const Scene::Document::ObjectNode& node);
    void updateItemFromNode(QTreeWidgetItem* item, const Scene::Document::ObjectNode& node);

    Scene::Document* document_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    bool updating_ = false;
};
