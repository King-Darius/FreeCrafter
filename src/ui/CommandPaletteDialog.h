#pragma once

#include <QDialog>
#include <QIcon>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <limits>

class HotkeyManager;
class ToolManager;
class QListView;
class QLineEdit;
class QStandardItemModel;

class CommandPaletteDialog : public QDialog {
    Q_OBJECT
public:
    struct Entry {
        QString id;
        QString title;
        QString detail;
        QString shortcut;
        QIcon icon;
        std::function<void()> callback;
    };

    explicit CommandPaletteDialog(QWidget* parent = nullptr);

    void populate(const HotkeyManager& hotkeys,
        ToolManager* toolManager,
        const QStringList& recentCommands,
        const std::function<QString(const QString&)>& toolHintResolver);

    void focusInput();

public slots:
    void activateCurrent();

signals:
    void commandExecuted(const QString& id, const QString& title, const QString& detail);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void handleFilterTextChanged(const QString& text);
    void handleItemActivated(const QModelIndex& index);

private:
    struct InternalEntry {
        Entry entry;
        int recentIndex = std::numeric_limits<int>::max();
    };

    void rebuildModel();
    void activateIndex(const QModelIndex& index);

    QListView* listView = nullptr;
    QLineEdit* searchEdit = nullptr;
    QStandardItemModel* model = nullptr;
    QVector<InternalEntry> entries;
    QString currentFilter;
};
