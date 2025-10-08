#include "LeftToolPalette.h"

#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSize>

LeftToolPalette::LeftToolPalette(const QList<ToolGroup>& groups, QWidget* parent)
    : QWidget(parent)
{
    auto* vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(6, 6, 6, 6);
    vbox->setSpacing(10);

    for (const auto& grp : groups) {
        if (grp.actions.isEmpty())
            continue;

        auto* label = new QLabel(grp.label, this);
        QFont font = label->font();
        font.setPointSize(font.pointSize() - 1);
        font.setBold(true);
        label->setFont(font);
        vbox->addWidget(label);

        auto* grid = new QGridLayout;
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(6);
        grid->setVerticalSpacing(6);

        int row = 0;
        int col = 0;
        for (auto* action : grp.actions) {
            if (!action)
                continue;
            auto* button = new QToolButton(this);
            button->setDefaultAction(action);
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            button->setAutoRaise(true);
            button->setIconSize(QSize(32, 32));
            button->setFocusPolicy(Qt::NoFocus);
            grid->addWidget(button, row, col);
            if (++col == 2) {
                col = 0;
                ++row;
            }
        }
        vbox->addLayout(grid);
    }

    vbox->addStretch();
}
