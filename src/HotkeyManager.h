#pragma once

#include <QObject>
#include <QHash>
#include <QKeySequence>
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
};
