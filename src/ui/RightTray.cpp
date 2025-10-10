#include "RightTray.h"

#include "CollapsibleSection.h"
#include "EnvironmentPanel.h"
#include "ExplorerPanel.h"
#include "HistoryPanel.h"
#include "InspectorPanel.h"

#include <QScrollArea>
#include <QVBoxLayout>

RightTray::RightTray(QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);

    auto* container = new QWidget;
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);

    inspector_ = new InspectorPanel(container);
    explorer_ = new ExplorerPanel(container);
    history_ = new HistoryPanel(undoStack, container);
    environment_ = new EnvironmentPanel(container);

    layout->addWidget(new CollapsibleSection(tr("Inspector"), inspector_, true));
    layout->addWidget(new CollapsibleSection(tr("Explorer"), explorer_, true));
    layout->addWidget(new CollapsibleSection(tr("History"), history_, true));
    layout->addWidget(new CollapsibleSection(tr("Environment"), environment_, true));
    layout->addStretch();

    scroll->setWidget(container);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
}
