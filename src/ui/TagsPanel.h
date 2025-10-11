#pragma once

#include <QColor>
#include <QWidget>

#include "Scene/Document.h"

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLineEdit;

class TagsPanel : public QWidget {
    Q_OBJECT
public:
    explicit TagsPanel(QWidget* parent = nullptr);

    void setDocument(Scene::Document* document);
    void refresh();

signals:
    void requestCreateTag(const QString& name, const QColor& color);
    void requestRenameTag(Scene::Document::TagId id, const QString& name);
    void requestSetTagVisibility(Scene::Document::TagId id, bool visible);
    void requestSetTagColor(Scene::Document::TagId id, const QColor& color);

private slots:
    void handleAddTag();
    void handleItemChanged(QTreeWidgetItem* item, int column);
    void handleItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void rebuild();
    QColor currentPickerColor() const;
    void setCurrentPickerColor(const QColor& color);

    Scene::Document* document_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QPushButton* colorButton_ = nullptr;
    QLineEdit* nameField_ = nullptr;
    QColor pendingColor_;
    bool updating_ = false;
};
