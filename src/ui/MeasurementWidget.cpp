#include "MeasurementWidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>

MeasurementWidget::MeasurementWidget(QWidget* parent)
    : QWidget(parent)
    , input(new QLineEdit(this))
    , unitSelector(new QComboBox(this))
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    input->setPlaceholderText(tr("Measurements"));
    unitSelector->addItem(tr("Architectural"), QStringLiteral("imperial"));
    unitSelector->addItem(tr("Metric"), QStringLiteral("metric"));
    unitSelector->setCurrentIndex(0);

    layout->addWidget(input, 1);
    layout->addWidget(unitSelector, 0);

    connect(input, &QLineEdit::returnPressed, this, &MeasurementWidget::handleReturnPressed);
}

QString MeasurementWidget::text() const
{
    return input->text();
}

void MeasurementWidget::setHint(const QString& hint)
{
    input->setPlaceholderText(hint);
}

QString MeasurementWidget::unitSystem() const
{
    return unitSelector->currentData().toString();
}

void MeasurementWidget::clear()
{
    input->clear();
}

void MeasurementWidget::handleReturnPressed()
{
    emit measurementCommitted(input->text(), unitSystem());
}
