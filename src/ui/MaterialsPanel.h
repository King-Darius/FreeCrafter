#pragma once

#include <QWidget>

#include "Scene/Document.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLineEdit;

namespace Core {
class CommandStack;
}

class MaterialsPanel : public QWidget {
    Q_OBJECT
public:
    explicit MaterialsPanel(QWidget* parent = nullptr);

    void setDocument(Scene::Document* document);
    void refresh();

signals:
    void assignMaterialRequested(const QString& materialName);

private slots:
    void handleItemActivated(QListWidgetItem* item);
    void handleApplyClicked();
    void handleSearchTextChanged(const QString& text);

private:
    void rebuildList();
    bool matchesFilter(const QString& material) const;

    Scene::Document* document_ = nullptr;
    QListWidget* list_ = nullptr;
    QPushButton* applyButton_ = nullptr;
    QLineEdit* searchField_ = nullptr;
    QString filterText_;
};
