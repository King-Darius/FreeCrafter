#include "HotkeyManager.h"

#include <QAction>
#include <QFile>
#include <QJsonValue>
#include <QJsonDocument>

#include <algorithm>

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
{
}

void HotkeyManager::registerAction(const QString& id, const QString& displayName, QAction* action, const QKeySequence& defaultSequence)
{
    auto it = std::find_if(registeredEntries.begin(), registeredEntries.end(), [&](const HotkeyEntry& entry) { return entry.id == id; });
    if (it == registeredEntries.end()) {
        HotkeyEntry entry;
        entry.id = id;
        entry.displayName = displayName;
        entry.action = action;
        entry.sequence = defaultSequence;
        registeredEntries.push_back(entry);
    } else {
        it->action = action;
        it->displayName = displayName;
        if (it->sequence.isEmpty()) {
            it->sequence = defaultSequence;
        }
    }

    if (action) {
        if (!defaultSequence.isEmpty() && action->shortcut().isEmpty()) {
            action->setShortcut(defaultSequence);
        } else if (!action->shortcut().isEmpty()) {
            // keep existing shortcut but ensure we track it
            auto iter = std::find_if(registeredEntries.begin(), registeredEntries.end(), [&](const HotkeyEntry& entry) { return entry.id == id; });
            if (iter != registeredEntries.end()) {
                iter->sequence = action->shortcut();
            }
        }
    }
}

bool HotkeyManager::loadFromResource(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    return loadFromJson(doc);
}

bool HotkeyManager::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    bool ok = loadFromJson(doc);
    if (ok) {
        emit hotkeysChanged();
    }
    return ok;
}

bool HotkeyManager::saveToFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QJsonDocument doc = toJson();
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool HotkeyManager::updateHotkey(const QString& id, const QKeySequence& sequence)
{
    for (auto& entry : registeredEntries) {
        if (entry.id == id) {
            entry.sequence = sequence;
            if (entry.action) {
                entry.action->setShortcut(sequence);
            }
            emit hotkeysChanged();
            return true;
        }
    }
    return false;
}

bool HotkeyManager::loadFromJson(const QJsonDocument& doc)
{
    if (!doc.isObject()) {
        return false;
    }
    QJsonObject root = doc.object();
    QJsonArray actions = root.value(QStringLiteral("actions")).toArray();
    if (actions.isEmpty()) {
        return false;
    }

    for (auto& entry : registeredEntries) {
        entry.sequence = QKeySequence();
    }

    for (const QJsonValue& value : actions) {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();
        QString id = obj.value(QStringLiteral("id")).toString();
        QString shortcut = obj.value(QStringLiteral("shortcut")).toString();
        if (id.isEmpty() || shortcut.isEmpty()) continue;
        QKeySequence sequence(shortcut);
        for (auto& entry : registeredEntries) {
            if (entry.id == id) {
                entry.sequence = sequence;
                break;
            }
        }
    }

    applySequences();
    return true;
}

QJsonDocument HotkeyManager::toJson() const
{
    QJsonArray actions;
    for (const auto& entry : registeredEntries) {
        if (entry.id.isEmpty()) continue;
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), entry.id);
        obj.insert(QStringLiteral("shortcut"), entry.sequence.toString());
        actions.append(obj);
    }

    QJsonObject root;
    root.insert(QStringLiteral("actions"), actions);
    return QJsonDocument(root);
}

void HotkeyManager::applySequences()
{
    for (auto& entry : registeredEntries) {
        if (entry.action) {
            entry.action->setShortcut(entry.sequence);
        }
    }
    emit hotkeysChanged();
}
