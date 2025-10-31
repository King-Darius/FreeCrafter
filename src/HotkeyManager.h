#pragma once

#include <QObject>
#include <QHash>
#include <QKeySequence>
#include <QList>
#include <memory>

class QAction;
class QWidget;

class HotkeyManager : public QObject
{
    Q_OBJECT
public:
    explicit HotkeyManager(QObject* parent = nullptr);

    void registerAction(const QString& id, QAction* action, const QString& label = QString());
    void showEditor(QWidget* parent);
    void importFromFile(const QString& path);
    void exportToFile(const QString& path) const;

    QHash<QString, QKeySequence> currentMap() const { return shortcuts; }

    struct CommandInfo {
        QString id;
        QAction* action = nullptr;
        QString label;
    };

    QList<CommandInfo> commands() const;

signals:
    void shortcutsChanged();

private:
    struct Binding {
        QAction* action = nullptr;
        QString label;
    };

    QHash<QString, Binding> bindings;
    QHash<QString, QKeySequence> shortcuts;
    QHash<QString, QKeySequence> defaultShortcuts;
    QString configPath;
    int schemaVersion = 1;

    void load();
    void save() const;
    void applyShortcut(const QString& id);
    void ensureConfigPath();
    void loadDefaults();
    bool readShortcuts(const QJsonObject& obj, QHash<QString, QKeySequence>& target) const;
    bool readShortcuts(const QJsonObject& obj);
    void writeShortcuts(QJsonObject& obj, const QHash<QString, QKeySequence>& source) const;
    void writeShortcuts(QJsonObject& obj) const;
};
