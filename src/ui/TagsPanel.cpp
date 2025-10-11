#include "TagsPanel.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QVariant>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
constexpr int kNameColumn = 0;
constexpr int kVisibilityColumn = 1;
constexpr int kColorColumn = 2;
constexpr int kTagIdRole = Qt::UserRole + 1;
}

TagsPanel::TagsPanel(QWidget* parent)
    : QWidget(parent)
    , pendingColor_(Qt::white)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* controls = new QWidget(this);
    auto* controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(4);

    nameField_ = new QLineEdit(controls);
    nameField_->setPlaceholderText(tr("New tag"));

    colorButton_ = new QPushButton(tr("Color"), controls);
    setCurrentPickerColor(pendingColor_);
    connect(colorButton_, &QPushButton::clicked, this, [this]() {
        QColor chosen = QColorDialog::getColor(pendingColor_, this, tr("Choose Tag Color"));
        if (chosen.isValid())
            setCurrentPickerColor(chosen);
    });

    auto* addButton = new QPushButton(tr("Add"), controls);
    connect(addButton, &QPushButton::clicked, this, &TagsPanel::handleAddTag);

    controlsLayout->addWidget(nameField_, 1);
    controlsLayout->addWidget(colorButton_);
    controlsLayout->addWidget(addButton);

    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(3);
    QStringList headers;
    headers << tr("Name") << tr("Visible") << tr("Color");
    tree_->setHeaderLabels(headers);
    tree_->header()->setSectionResizeMode(kNameColumn, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(kVisibilityColumn, QHeaderView::ResizeToContents);
    tree_->header()->setSectionResizeMode(kColorColumn, QHeaderView::ResizeToContents);
    tree_->setRootIsDecorated(false);
    tree_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    connect(tree_, &QTreeWidget::itemChanged, this, &TagsPanel::handleItemChanged);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, &TagsPanel::handleItemDoubleClicked);

    layout->addWidget(controls);
    layout->addWidget(tree_, 1);
}

void TagsPanel::setDocument(Scene::Document* document)
{
    if (document_ == document)
        return;
    document_ = document;
    refresh();
}

void TagsPanel::refresh()
{
    rebuild();
}

void TagsPanel::handleAddTag()
{
    if (!document_)
        return;
    const QString name = nameField_->text().trimmed();
    if (name.isEmpty())
        return;
    emit requestCreateTag(name, pendingColor_);
    nameField_->clear();
}

void TagsPanel::handleItemChanged(QTreeWidgetItem* item, int column)
{
    if (updating_ || !item)
        return;
    Scene::Document::TagId tagId = static_cast<Scene::Document::TagId>(item->data(kNameColumn, kTagIdRole).toULongLong());
    if (tagId == 0)
        return;

    if (column == kNameColumn) {
        emit requestRenameTag(tagId, item->text(kNameColumn));
    } else if (column == kVisibilityColumn) {
        bool visible = item->checkState(kVisibilityColumn) == Qt::Checked;
        emit requestSetTagVisibility(tagId, visible);
    }
}

void TagsPanel::handleItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    if (!item || column != kColorColumn)
        return;
    Scene::Document::TagId tagId = static_cast<Scene::Document::TagId>(item->data(kNameColumn, kTagIdRole).toULongLong());
    if (tagId == 0)
        return;
    QColor current = item->data(kColorColumn, Qt::DecorationRole).value<QColor>();
    QColor chosen = QColorDialog::getColor(current, this, tr("Choose Tag Color"));
    if (chosen.isValid())
        emit requestSetTagColor(tagId, chosen);
}

void TagsPanel::rebuild()
{
    if (!tree_)
        return;

    updating_ = true;
    tree_->clear();

    if (!document_) {
        updating_ = false;
        return;
    }

    const auto& tags = document_->tags();
    for (const auto& entry : tags) {
        const Scene::Document::Tag& tag = entry.second;
        auto* item = new QTreeWidgetItem(tree_);
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setText(kNameColumn, QString::fromStdString(tag.name));
        item->setData(kNameColumn, kTagIdRole, QVariant::fromValue(static_cast<qulonglong>(tag.id)));
        item->setCheckState(kVisibilityColumn, tag.visible ? Qt::Checked : Qt::Unchecked);
        QColor color(tag.color.r * 255.0f, tag.color.g * 255.0f, tag.color.b * 255.0f);
        item->setData(kColorColumn, Qt::DecorationRole, color);
        item->setText(kColorColumn, color.name());
    }

    updating_ = false;
}

QColor TagsPanel::currentPickerColor() const
{
    return pendingColor_;
}

void TagsPanel::setCurrentPickerColor(const QColor& color)
{
    pendingColor_ = color;
    if (colorButton_) {
        QString style = QStringLiteral("background-color: %1").arg(color.name());
        colorButton_->setStyleSheet(style);
    }
}
