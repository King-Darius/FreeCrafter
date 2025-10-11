#include "OutlinerPanel.h"

#include <QHeaderView>
#include <QVariant>
#include <QVBoxLayout>

#include "Core/CommandStack.h"

namespace {
constexpr int kNameColumn = 0;
constexpr int kVisibilityColumn = 1;
constexpr int kIdRole = Qt::UserRole + 1;
}

OutlinerPanel::OutlinerPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(2);
    QStringList headers;
    headers << tr("Name") << tr("Visible");
    tree_->setHeaderLabels(headers);
    tree_->header()->setSectionResizeMode(kNameColumn, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(kVisibilityColumn, QHeaderView::ResizeToContents);
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    tree_->setAllColumnsShowFocus(true);
    tree_->setUniformRowHeights(true);

    connect(tree_, &QTreeWidget::itemChanged, this, &OutlinerPanel::handleItemChanged);
    connect(tree_, &QTreeWidget::itemExpanded, this, &OutlinerPanel::handleItemExpanded);
    connect(tree_, &QTreeWidget::itemCollapsed, this, &OutlinerPanel::handleItemCollapsed);

    layout->addWidget(tree_);
}

void OutlinerPanel::setDocument(Scene::Document* document)
{
    if (document_ == document)
        return;
    document_ = document;
    rebuildTree();
}

void OutlinerPanel::refresh()
{
    rebuildTree();
}

std::vector<Scene::Document::ObjectId> OutlinerPanel::selectedObjectIds() const
{
    std::vector<Scene::Document::ObjectId> ids;
    if (!tree_)
        return ids;
    const auto items = tree_->selectedItems();
    ids.reserve(items.size());
    for (QTreeWidgetItem* item : items) {
        Scene::Document::ObjectId id = static_cast<Scene::Document::ObjectId>(item->data(kNameColumn, kIdRole).toULongLong());
        if (id != 0)
            ids.push_back(id);
    }
    return ids;
}

void OutlinerPanel::setCommandStack(Core::CommandStack* stack)
{
    Q_UNUSED(stack);
}

void OutlinerPanel::handleItemChanged(QTreeWidgetItem* item, int column)
{
    if (updating_ || !document_ || !item)
        return;

    Scene::Document::ObjectId id = static_cast<Scene::Document::ObjectId>(item->data(kNameColumn, kIdRole).toULongLong());
    if (id == 0)
        return;

    if (column == kNameColumn) {
        emit requestRename(id, item->text(kNameColumn));
    } else if (column == kVisibilityColumn) {
        bool visible = item->checkState(kVisibilityColumn) == Qt::Checked;
        emit requestVisibilityChange(id, visible);
    }
}

void OutlinerPanel::handleItemExpanded(QTreeWidgetItem* item)
{
    if (!document_ || !item)
        return;
    Scene::Document::ObjectId id = static_cast<Scene::Document::ObjectId>(item->data(kNameColumn, kIdRole).toULongLong());
    if (id == 0)
        return;
    emit requestExpansionChange(id, true);
}

void OutlinerPanel::handleItemCollapsed(QTreeWidgetItem* item)
{
    if (!document_ || !item)
        return;
    Scene::Document::ObjectId id = static_cast<Scene::Document::ObjectId>(item->data(kNameColumn, kIdRole).toULongLong());
    if (id == 0)
        return;
    emit requestExpansionChange(id, false);
}

void OutlinerPanel::rebuildTree()
{
    if (!tree_)
        return;
    updating_ = true;
    tree_->clear();
    if (!document_) {
        updating_ = false;
        return;
    }

    const Scene::Document::ObjectNode& root = document_->objectTree();
    for (const auto& child : root.children) {
        populateNode(nullptr, *child);
    }
    updating_ = false;
}

void OutlinerPanel::populateNode(QTreeWidgetItem* parentItem, const Scene::Document::ObjectNode& node)
{
    QTreeWidgetItem* item = nullptr;
    if (parentItem) {
        item = new QTreeWidgetItem(parentItem);
    } else {
        item = new QTreeWidgetItem(tree_);
    }
    item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    updateItemFromNode(item, node);

    for (const auto& child : node.children) {
        populateNode(item, *child);
    }

    if (node.expanded) {
        tree_->expandItem(item);
    } else {
        tree_->collapseItem(item);
    }
}

void OutlinerPanel::updateItemFromNode(QTreeWidgetItem* item, const Scene::Document::ObjectNode& node)
{
    item->setText(kNameColumn, QString::fromStdString(node.name));
    item->setData(kNameColumn, kIdRole, QVariant::fromValue(static_cast<qulonglong>(node.id)));
    item->setCheckState(kVisibilityColumn, node.visible ? Qt::Checked : Qt::Unchecked);
}
