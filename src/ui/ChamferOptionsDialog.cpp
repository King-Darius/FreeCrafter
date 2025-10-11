#include "ChamferOptionsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>

#include <algorithm>

ChamferOptionsDialog::ChamferOptionsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Chamfer Options"));
    setModal(true);

    auto* layout = new QFormLayout(this);
    radiusSpin_ = new QDoubleSpinBox(this);
    radiusSpin_->setRange(0.001, 1000.0);
    radiusSpin_->setDecimals(3);
    radiusSpin_->setSingleStep(0.01);
    layout->addRow(tr("Radius"), radiusSpin_);

    segmentsSpin_ = new QSpinBox(this);
    segmentsSpin_->setRange(1, 128);
    segmentsSpin_->setSingleStep(1);
    layout->addRow(tr("Segments"), segmentsSpin_);

    styleCombo_ = new QComboBox(this);
    styleCombo_->addItem(tr("Fillet"), static_cast<int>(Phase6::CornerStyle::Fillet));
    styleCombo_->addItem(tr("Chamfer"), static_cast<int>(Phase6::CornerStyle::Chamfer));
    layout->addRow(tr("Style"), styleCombo_);

    hardEdgeCheck_ = new QCheckBox(tr("Tag resulting edges as hard"), this);
    layout->addRow(QString(), hardEdgeCheck_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ChamferOptionsDialog::setOptions(const Phase6::RoundCornerOptions& options)
{
    radiusSpin_->setValue(options.radius);
    segmentsSpin_->setValue(std::max(1, options.segments));
    int styleIndex = styleCombo_->findData(static_cast<int>(options.style));
    if (styleIndex < 0)
        styleIndex = 0;
    styleCombo_->setCurrentIndex(styleIndex);
    hardEdgeCheck_->setChecked(options.tagHardEdges);
}

Phase6::RoundCornerOptions ChamferOptionsDialog::options() const
{
    Phase6::RoundCornerOptions opts;
    opts.radius = static_cast<float>(radiusSpin_->value());
    opts.segments = segmentsSpin_->value();
    opts.style = static_cast<Phase6::CornerStyle>(styleCombo_->currentData().toInt());
    opts.tagHardEdges = hardEdgeCheck_->isChecked();
    return opts;
}
