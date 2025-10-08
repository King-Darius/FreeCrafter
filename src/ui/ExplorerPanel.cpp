#include "ExplorerPanel.h"

#include <QLabel>
#include <QVBoxLayout>
#include <Qt>

ExplorerPanel::ExplorerPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* label = new QLabel(tr("Explorer coming soon"), this);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);

    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();
}
