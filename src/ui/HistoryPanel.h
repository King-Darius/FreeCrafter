#pragma once

#include <QWidget>

class QLabel;
class QUndoStack;
class QUndoView;

class HistoryPanel : public QWidget {
    Q_OBJECT
public:
    explicit HistoryPanel(QUndoStack* stack, QWidget* parent = nullptr);

    void setUndoStack(QUndoStack* stack);

private:
    void updateVisibility();

    QUndoStack* undoStack_ = nullptr;
    QUndoView* view_ = nullptr;
    QLabel* placeholder_ = nullptr;
};
