#pragma once

#include <QWidget>

class QUndoStack;
class InspectorPanel;
class ExplorerPanel;
class HistoryPanel;
class EnvPanel;

class RightTray : public QWidget {
    Q_OBJECT
public:
    explicit RightTray(QUndoStack* undoStack, QWidget* parent = nullptr);

    InspectorPanel* inspectorPanel() const { return inspector_; }
    ExplorerPanel* explorerPanel() const { return explorer_; }
    HistoryPanel* historyPanel() const { return history_; }
    EnvPanel* environmentPanel() const { return environment_; }

private:
    InspectorPanel* inspector_ = nullptr;
    ExplorerPanel* explorer_ = nullptr;
    HistoryPanel* history_ = nullptr;
    EnvPanel* environment_ = nullptr;
};
