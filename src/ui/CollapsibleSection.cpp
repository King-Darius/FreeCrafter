#include "CollapsibleSection.h"

#include <QFrame>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSizePolicy>

CollapsibleSection::CollapsibleSection(const QString& title, QWidget* content,
                                       bool open, QWidget* parent)
    : QWidget(parent)
    , content_(content)
    , open_(open)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    auto* header = new QToolButton(this);
    header->setText(title);
    header->setCheckable(true);
    header->setChecked(open_);
    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    header->setArrowType(open_ ? Qt::DownArrow : Qt::RightArrow);
    header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    connect(header, &QToolButton::toggled, this, [this, header](bool on) {
        open_ = on;
        if (content_)
            content_->setVisible(on);
        header->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
        emit toggled(on);
    });

    auto* card = new QFrame(this);
    card->setProperty("role", "Card");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->addWidget(content_);

    if (content_)
        content_->setVisible(open_);

    layout->addWidget(header);
    layout->addWidget(card);
}
