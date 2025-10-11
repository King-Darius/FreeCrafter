#include "LoftOptionsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>

#include <algorithm>

LoftOptionsDialog::LoftOptionsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Loft Options"));
    setModal(true);

    auto* layout = new QFormLayout(this);

    sectionsSpin_ = new QSpinBox(this);
    sectionsSpin_->setRange(2, 256);
    sectionsSpin_->setSingleStep(1);
    layout->addRow(tr("Sections"), sectionsSpin_);

    twistSpin_ = new QDoubleSpinBox(this);
    twistSpin_->setRange(-360.0, 360.0);
    twistSpin_->setDecimals(2);
    twistSpin_->setSingleStep(5.0);
    layout->addRow(tr("Twist (°)"), twistSpin_);

    smoothingSpin_ = new QSpinBox(this);
    smoothingSpin_->setRange(0, 10);
    smoothingSpin_->setSingleStep(1);
    layout->addRow(tr("Smoothing Passes"), smoothingSpin_);

    closeRailsCheck_ = new QCheckBox(tr("Close rails"), this);
    layout->addRow(QString(), closeRailsCheck_);

    smoothNormalsCheck_ = new QCheckBox(tr("Smooth normals"), this);
    layout->addRow(QString(), smoothNormalsCheck_);

    symmetricPairCheck_ = new QCheckBox(tr("Symmetric pairing"), this);
    layout->addRow(QString(), symmetricPairCheck_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void LoftOptionsDialog::setOptions(const Phase6::LoftOptions& options)
{
    sectionsSpin_->setValue(std::max(2, options.sections));
    twistSpin_->setValue(options.twistDegrees);
    smoothingSpin_->setValue(std::max(0, options.smoothingPasses));
    closeRailsCheck_->setChecked(options.closeRails);
    smoothNormalsCheck_->setChecked(options.smoothNormals);
    symmetricPairCheck_->setChecked(options.symmetricPairing);
}

Phase6::LoftOptions LoftOptionsDialog::options() const
{
    Phase6::LoftOptions opts;
    opts.sections = sectionsSpin_->value();
    opts.twistDegrees = static_cast<float>(twistSpin_->value());
    opts.smoothingPasses = smoothingSpin_->value();
    opts.closeRails = closeRailsCheck_->isChecked();
    opts.smoothNormals = smoothNormalsCheck_->isChecked();
    opts.symmetricPairing = symmetricPairCheck_->isChecked();
    return opts;
}
