#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointer>
#include <QVector>

#include <algorithm>

class QAction;

struct HotkeyEntry
{
    QString id;
    QString displayName;
    QPointer<QAction> action;
    QKeySequence sequence;
};

class HotkeyManager : public QObject
{
    Q_OBJECT
public:
    explicit HotkeyManager(QObject* parent = nullptr);

    void registerAction(const QString& id, const QString& displayName, QAction* action, const QKeySequence& defaultSequence = QKeySequence());

    bool loadFromResource(const QString& resourcePath);
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath) const;

    QVector<HotkeyEntry> entries() const { return registeredEntries; }

    bool updateHotkey(const QString& id, const QKeySequence& sequence);

signals:
    void hotkeysChanged();

private:
    bool loadFromJson(const QJsonDocument& doc);
    QJsonDocument toJson() const;
    void applySequences();

    QVector<HotkeyEntry> registeredEntries;
};
