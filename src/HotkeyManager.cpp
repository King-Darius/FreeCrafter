#include "HotkeyManager.h"

#include <QAction>
#include <QAbstractItemView>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSet>
#include <algorithm>

namespace {
constexpr auto kConfigFileName = "hotkeys.json";
constexpr auto kDefaultResource = ":/config/hotkeys_default.json";
}

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
{
    ensureConfigPath();
    loadDefaults();
    load();
}

void HotkeyManager::registerAction(const QString& id, QAction* action, const QString& label)
{
    if (!action)
        return;

    Binding binding;
    binding.action = action;
    binding.label = label.isEmpty() ? action->text() : label;
    bindings.insert(id, binding);

    if (shortcuts.contains(id)) {
        action->setShortcut(shortcuts.value(id));
    }

    action->setProperty("hotkeyId", id);
}

QList<HotkeyManager::CommandInfo> HotkeyManager::commands() const
{
    QList<CommandInfo> list;
    list.reserve(bindings.size());

    for (auto it = bindings.constBegin(); it != bindings.constEnd(); ++it) {
        CommandInfo info;
        info.id = it.key();
        info.action = it.value().action;
        info.label = it.value().label;
        list.append(info);
    }

    return list;
}

void HotkeyManager::showEditor(QWidget* parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(tr("Customize Shortcuts"));
    dialog.resize(520, 480);

    auto* mainLayout = new QVBoxLayout(&dialog);
    auto* infoLabel = new QLabel(tr("Double-click a shortcut to edit. Conflicts are highlighted."));
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    QTableWidget* table = new QTableWidget(bindings.size(), 2, &dialog);
    table->setHorizontalHeaderLabels({tr("Action"), tr("Shortcut")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QList<QString> ids = bindings.keys();
    std::sort(ids.begin(), ids.end(), [this](const QString& a, const QString& b) {
        return bindings.value(a).label.toLower() < bindings.value(b).label.toLower();
    });

    int row = 0;
    for (const QString& id : ids) {
        const Binding& binding = bindings.value(id);
        auto* labelItem = new QTableWidgetItem(binding.label);
        table->setItem(row, 0, labelItem);

        auto* editor = new QKeySequenceEdit(table);
        editor->setKeySequence(shortcuts.value(id));
        editor->setProperty("bindingId", id);
        table->setCellWidget(row, 1, editor);

        row++;
    }

    mainLayout->addWidget(table);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto* importButton = new QPushButton(tr("Import…"));
    auto* exportButton = new QPushButton(tr("Export…"));
    auto* resetButton = new QPushButton(tr("Reset to Defaults"));
    auto* cancelButton = new QPushButton(tr("Cancel"));
    auto* applyButton = new QPushButton(tr("Save"));

    buttonRow->addWidget(importButton);
    buttonRow->addWidget(exportButton);
    buttonRow->addWidget(resetButton);
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(applyButton);
    mainLayout->addLayout(buttonRow);

    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    QObject::connect(resetButton, &QPushButton::clicked, [&]() {
        for (int r = 0; r < table->rowCount(); ++r) {
            if (auto* editor = qobject_cast<QKeySequenceEdit*>(table->cellWidget(r, 1))) {
                const QString id = editor->property("bindingId").toString();
                editor->setKeySequence(defaultShortcuts.value(id));
            }
        }
    });

    QObject::connect(importButton, &QPushButton::clicked, [&]() {
        const QString path = QFileDialog::getOpenFileName(&dialog, tr("Import Hotkeys"), QString(), tr("JSON (*.json)"));
        if (path.isEmpty())
            return;
        importFromFile(path);
        for (int r = 0; r < table->rowCount(); ++r) {
            if (auto* editor = qobject_cast<QKeySequenceEdit*>(table->cellWidget(r, 1))) {
                const QString id = editor->property("bindingId").toString();
                editor->setKeySequence(shortcuts.value(id));
            }
        }
    });

    QObject::connect(exportButton, &QPushButton::clicked, [&]() {
        const QString path = QFileDialog::getSaveFileName(&dialog, tr("Export Hotkeys"), QStringLiteral("hotkeys.json"), tr("JSON (*.json)"));
        if (path.isEmpty())
            return;
        exportToFile(path);
    });

    QObject::connect(applyButton, &QPushButton::clicked, [&]() {
        QHash<QString, QKeySequence> updated = shortcuts;
        QSet<QString> usedSequences;

        // Pre-populate with shortcuts that belong to non-editable bindings
        for (auto it = updated.constBegin(); it != updated.constEnd(); ++it) {
            if (bindings.contains(it.key()))
                continue;
            const QString portable = it.value().toString(QKeySequence::PortableText);
            if (!portable.isEmpty())
                usedSequences.insert(portable);
        }

        for (int r = 0; r < table->rowCount(); ++r) {
            if (auto* editor = qobject_cast<QKeySequenceEdit*>(table->cellWidget(r, 1))) {
                const QString id = editor->property("bindingId").toString();
                const QString oldPortable = updated.value(id).toString(QKeySequence::PortableText);
                if (!oldPortable.isEmpty())
                    usedSequences.remove(oldPortable);

                const QKeySequence seq = editor->keySequence();
                const QString portable = seq.toString(QKeySequence::PortableText);
                if (!portable.isEmpty() && usedSequences.contains(portable)) {
                    QMessageBox::warning(&dialog, tr("Shortcut Conflict"),
                                          tr("%1 conflicts with an existing shortcut. Please choose a unique combination.")
                                              .arg(seq.toString(QKeySequence::NativeText)));
                    return;
                }

                if (!portable.isEmpty())
                    usedSequences.insert(portable);

                updated.insert(id, seq);
            }
        }
        shortcuts = updated;
        save();
        for (auto it = shortcuts.begin(); it != shortcuts.end(); ++it) {
            applyShortcut(it.key());
        }
        emit shortcutsChanged();
        dialog.accept();
    });

    dialog.exec();
}

void HotkeyManager::importFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const auto doc = QJsonDocument::fromJson(file.readAll());
    QHash<QString, QKeySequence> updated = shortcuts;
    if (!readShortcuts(doc.object(), updated))
        return;

    shortcuts = updated;
    save();
    for (auto it = shortcuts.begin(); it != shortcuts.end(); ++it)
        applyShortcut(it.key());
    emit shortcutsChanged();
}

void HotkeyManager::exportToFile(const QString& path) const
{
    QJsonObject obj;
    writeShortcuts(obj);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(obj).toJson());
}

void HotkeyManager::load()
{
    shortcuts = defaultShortcuts;

    QFile file(configPath);
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly))
        return;

    const auto doc = QJsonDocument::fromJson(file.readAll());
    readShortcuts(doc.object(), shortcuts);
}

void HotkeyManager::save() const
{
    QJsonObject obj;
    writeShortcuts(obj);

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(obj).toJson());
}

void HotkeyManager::applyShortcut(const QString& id)
{
    if (!bindings.contains(id))
        return;
    QAction* action = bindings.value(id).action;
    if (!action)
        return;
    action->setShortcut(shortcuts.value(id));
}

void HotkeyManager::ensureConfigPath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(base);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    configPath = dir.filePath(QString::fromLatin1(kConfigFileName));
}

void HotkeyManager::loadDefaults()
{
    QFile file(QString::fromLatin1(kDefaultResource));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const auto doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject obj = doc.object();
    schemaVersion = obj.value(QStringLiteral("version")).toInt(schemaVersion);

    QHash<QString, QKeySequence> defaults = defaultShortcuts;
    if (readShortcuts(obj, defaults)) {
        defaultShortcuts = defaults;
    }
    shortcuts = defaultShortcuts;
}

bool HotkeyManager::readShortcuts(const QJsonObject& obj, QHash<QString, QKeySequence>& target) const
{
    if (obj.isEmpty())
        return false;

    const int version = obj.value(QStringLiteral("version")).toInt(schemaVersion);
    if (version != schemaVersion)
        return false;

    const QJsonObject actions = obj.value(QStringLiteral("actions")).toObject();
    for (auto it = actions.begin(); it != actions.end(); ++it) {
        target.insert(it.key(), QKeySequence(it.value().toString()));
    }
    return true;
}

bool HotkeyManager::readShortcuts(const QJsonObject& obj)
{
    return readShortcuts(obj, shortcuts);
}

void HotkeyManager::writeShortcuts(QJsonObject& obj, const QHash<QString, QKeySequence>& source) const
{
    obj.insert(QStringLiteral("version"), schemaVersion);

    QJsonObject actions;
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        actions.insert(it.key(), it.value().toString(QKeySequence::PortableText));
    }
    obj.insert(QStringLiteral("actions"), actions);
}

void HotkeyManager::writeShortcuts(QJsonObject& obj) const
{
    writeShortcuts(obj, shortcuts);
}
