#include "HistoryPanel.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QUndoStack>
#include <QUndoView>
#include <Qt>

HistoryPanel::HistoryPanel(QUndoStack* stack, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    view_ = new QUndoView(this);
    view_->setStack(stack);
    view_->setEmptyLabel(tr("No actions yet"));

    placeholder_ = new QLabel(tr("History unavailable"), this);
    placeholder_->setAlignment(Qt::AlignCenter);
    placeholder_->setWordWrap(true);

    layout->addWidget(view_);
    layout->addWidget(placeholder_);

    setUndoStack(stack);
}

void HistoryPanel::setUndoStack(QUndoStack* stack)
{
    undoStack_ = stack;
    if (view_)
        view_->setStack(stack);
    updateVisibility();
}

void HistoryPanel::updateVisibility()
{
    const bool hasStack = undoStack_ != nullptr;
    if (view_)
        view_->setVisible(hasStack);
    if (placeholder_)
        placeholder_->setVisible(!hasStack);
}

void HistoryPanel::refresh()
{
    if (view_)
        view_->setStack(undoStack_);
    updateVisibility();
}
