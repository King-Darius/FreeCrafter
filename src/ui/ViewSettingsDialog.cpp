#include "ui/ViewSettingsDialog.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr float kMinFov = 15.0f;
constexpr float kMaxFov = 120.0f;
}

ViewSettingsDialog::ViewSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Camera Settings"));
    resize(320, 200);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto* projectionLabel = new QLabel(tr("Projection"), this);
    projectionLabel->setWordWrap(true);
    layout->addWidget(projectionLabel);

    perspectiveButton = new QRadioButton(tr("Perspective"), this);
    parallelButton = new QRadioButton(tr("Parallel"), this);
    perspectiveButton->setChecked(true);

    auto* projectionGroup = new QButtonGroup(this);
    projectionGroup->addButton(perspectiveButton);
    projectionGroup->addButton(parallelButton);

    layout->addWidget(perspectiveButton);
    layout->addWidget(parallelButton);

    auto* fovLayout = new QFormLayout();
    fovLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    fovSpin = new QDoubleSpinBox(this);
    fovSpin->setRange(kMinFov, kMaxFov);
    fovSpin->setSuffix(QStringLiteral(" °"));
    fovSpin->setDecimals(1);
    fovSpin->setSingleStep(1.0);
    fovSpin->setValue(60.0);
    fovLayout->addRow(tr("Field of view"), fovSpin);
    layout->addLayout(fovLayout);

    connect(parallelButton, &QRadioButton::toggled, this, [this](bool checked) {
        fovSpin->setEnabled(!checked);
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ViewSettingsDialog::setFieldOfView(float fovDegrees)
{
    fovSpin->setValue(std::clamp(fovDegrees, kMinFov, kMaxFov));
}

float ViewSettingsDialog::fieldOfView() const
{
    return static_cast<float>(fovSpin->value());
}

void ViewSettingsDialog::setProjectionMode(CameraController::ProjectionMode mode)
{
    const bool isParallel = mode == CameraController::ProjectionMode::Parallel;
    parallelButton->setChecked(isParallel);
    perspectiveButton->setChecked(!isParallel);
    fovSpin->setEnabled(!isParallel);
}

CameraController::ProjectionMode ViewSettingsDialog::projectionMode() const
{
    return parallelButton->isChecked() ? CameraController::ProjectionMode::Parallel
                                       : CameraController::ProjectionMode::Perspective;
}

