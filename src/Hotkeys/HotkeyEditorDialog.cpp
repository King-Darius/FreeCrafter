#include "HotkeyEditorDialog.h"

#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

HotkeyEditorDialog::HotkeyEditorDialog(HotkeyManager* manager, QWidget* parent)
    : QDialog(parent)
    , manager(manager)
{
    setWindowTitle(tr("Hotkey Editor"));
    resize(420, 320);

    auto* layout = new QVBoxLayout(this);
    auto* description = new QLabel(tr("Double-click a shortcut cell to edit, then press Apply."), this);
    description->setWordWrap(true);
    layout->addWidget(description);

    table = new QTableWidget(this);
    table->setColumnCount(2);
    QStringList headers;
    headers << tr("Action") << tr("Shortcut");
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(table, 1);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto* applyButton = new QPushButton(tr("Apply"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);
    buttonRow->addWidget(applyButton);
    buttonRow->addWidget(closeButton);
    layout->addLayout(buttonRow);

    connect(applyButton, &QPushButton::clicked, this, &HotkeyEditorDialog::applyChanges);
    connect(closeButton, &QPushButton::clicked, this, &HotkeyEditorDialog::accept);

    populateFromManager();
}

void HotkeyEditorDialog::populateFromManager()
{
    table->setRowCount(0);
    if (!manager) {
        return;
    }

    const auto rows = manager->entries();
    table->setRowCount(rows.size());
    int row = 0;
    for (const auto& entry : rows) {
        auto* nameItem = new QTableWidgetItem(entry.displayName);
        nameItem->setFlags(Qt::ItemIsEnabled);
        table->setItem(row, 0, nameItem);

        auto* edit = new QKeySequenceEdit(entry.sequence, table);
        table->setCellWidget(row, 1, edit);
        edit->setProperty("hotkeyId", entry.id);
        ++row;
    }
}

void HotkeyEditorDialog::applyChanges()
{
    if (!manager) return;
    const int rows = table->rowCount();
    for (int row = 0; row < rows; ++row) {
        QWidget* widget = table->cellWidget(row, 1);
        auto* edit = qobject_cast<QKeySequenceEdit*>(widget);
        if (!edit) continue;
        QString id = edit->property("hotkeyId").toString();
        manager->updateHotkey(id, edit->keySequence());
    }
}
