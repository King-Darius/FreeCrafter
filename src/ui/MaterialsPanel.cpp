#include "MaterialsPanel.h"

#include <algorithm>
#include <unordered_set>

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "GeometryKernel/GeometryKernel.h"

MaterialsPanel::MaterialsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    searchField_ = new QLineEdit(this);
    searchField_->setPlaceholderText(tr("Search materials"));
    connect(searchField_, &QLineEdit::textChanged, this, &MaterialsPanel::handleSearchTextChanged);

    list_ = new QListWidget(this);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(list_, &QListWidget::itemActivated, this, &MaterialsPanel::handleItemActivated);

    applyButton_ = new QPushButton(tr("Apply to Selection"), this);
    applyButton_->setEnabled(false);
    connect(applyButton_, &QPushButton::clicked, this, &MaterialsPanel::handleApplyClicked);

    layout->addWidget(searchField_);
    layout->addWidget(list_, 1);
    layout->addWidget(applyButton_);
}

void MaterialsPanel::setDocument(Scene::Document* document)
{
    if (document_ == document)
        return;
    document_ = document;
    refresh();
}

void MaterialsPanel::refresh()
{
    rebuildList();
}

void MaterialsPanel::handleItemActivated(QListWidgetItem* item)
{
    if (!item)
        return;
    emit assignMaterialRequested(item->text());
}

void MaterialsPanel::handleApplyClicked()
{
    if (!list_)
        return;
    QListWidgetItem* current = list_->currentItem();
    if (!current)
        return;
    emit assignMaterialRequested(current->text());
}

void MaterialsPanel::handleSearchTextChanged(const QString& text)
{
    filterText_ = text.trimmed();
    rebuildList();
}

void MaterialsPanel::rebuildList()
{
    if (!list_)
        return;

    list_->clear();
    applyButton_->setEnabled(false);

    if (!document_)
        return;

    std::unordered_set<QString> unique;
    const auto& assignments = document_->geometry().getMaterials();
    unique.reserve(assignments.size());
    for (const auto& entry : assignments) {
        QString name = QString::fromStdString(entry.second);
        if (!matchesFilter(name))
            continue;
        unique.insert(name);
    }

    std::vector<QString> sorted(unique.begin(), unique.end());
    std::sort(sorted.begin(), sorted.end(), [](const QString& a, const QString& b) {
        return a.localeAwareCompare(b) < 0;
    });

    for (const QString& material : sorted) {
        auto* item = new QListWidgetItem(material, list_);
        (void)item;
    }

    if (!sorted.empty()) {
        list_->setCurrentRow(0);
        applyButton_->setEnabled(true);
    }
}

bool MaterialsPanel::matchesFilter(const QString& material) const
{
    if (filterText_.isEmpty())
        return true;
    return material.contains(filterText_, Qt::CaseInsensitive);
}
