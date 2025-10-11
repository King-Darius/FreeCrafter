#include "ui/ViewSettingsDialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr float kMinFov = 15.0f;
constexpr float kMaxFov = 120.0f;
}

ViewSettingsDialog::ViewSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("View Settings"));
    resize(360, 420);

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

    auto* gridGroup = new QGroupBox(tr("Grid"), this);
    auto* gridLayout = new QFormLayout(gridGroup);
    gridLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    gridSpacingSpin = new QDoubleSpinBox(gridGroup);
    gridSpacingSpin->setRange(0.001, 1000.0);
    gridSpacingSpin->setDecimals(3);
    gridSpacingSpin->setSingleStep(0.1);
    gridSpacingSpin->setValue(1.0);
    gridSpacingSpin->setSuffix(tr(" units"));
    gridLayout->addRow(tr("Major spacing"), gridSpacingSpin);

    gridDivisionsSpin = new QSpinBox(gridGroup);
    gridDivisionsSpin->setRange(1, 64);
    gridDivisionsSpin->setValue(4);
    gridLayout->addRow(tr("Divisions per major"), gridDivisionsSpin);

    gridExtentSpin = new QSpinBox(gridGroup);
    gridExtentSpin->setRange(1, 500);
    gridExtentSpin->setValue(50);
    gridLayout->addRow(tr("Major count"), gridExtentSpin);

    layout->addWidget(gridGroup);

    auto* shadowGroup = new QGroupBox(tr("Shadows"), this);
    auto* shadowLayout = new QFormLayout(shadowGroup);
    shadowLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    shadowEnableCheck = new QCheckBox(tr("Enable shadows"), shadowGroup);
    shadowEnableCheck->setChecked(false);
    shadowLayout->addRow(QString(), shadowEnableCheck);

    shadowQualityCombo = new QComboBox(shadowGroup);
    shadowQualityCombo->addItem(tr("Low"), static_cast<int>(Scene::SceneSettings::ShadowQuality::Low));
    shadowQualityCombo->addItem(tr("Medium"), static_cast<int>(Scene::SceneSettings::ShadowQuality::Medium));
    shadowQualityCombo->addItem(tr("High"), static_cast<int>(Scene::SceneSettings::ShadowQuality::High));
    shadowQualityCombo->setCurrentIndex(1);
    shadowLayout->addRow(tr("Quality"), shadowQualityCombo);

    shadowStrengthSpin = new QDoubleSpinBox(shadowGroup);
    shadowStrengthSpin->setRange(0.0, 100.0);
    shadowStrengthSpin->setSingleStep(5.0);
    shadowStrengthSpin->setDecimals(0);
    shadowStrengthSpin->setSuffix(QStringLiteral(" %"));
    shadowStrengthSpin->setValue(65.0);
    shadowLayout->addRow(tr("Darkness"), shadowStrengthSpin);

    shadowBiasSpin = new QDoubleSpinBox(shadowGroup);
    shadowBiasSpin->setDecimals(4);
    shadowBiasSpin->setRange(0.0001, 0.02);
    shadowBiasSpin->setSingleStep(0.0005);
    shadowBiasSpin->setValue(0.0035);
    shadowLayout->addRow(tr("Bias"), shadowBiasSpin);

    layout->addWidget(shadowGroup);

    auto updateShadowControls = [this]() {
        const bool enabled = shadowEnableCheck->isChecked();
        shadowQualityCombo->setEnabled(enabled);
        shadowStrengthSpin->setEnabled(enabled);
        shadowBiasSpin->setEnabled(enabled);
    };
    connect(shadowEnableCheck, &QCheckBox::toggled, this, [updateShadowControls]() { updateShadowControls(); });
    updateShadowControls();

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

void ViewSettingsDialog::setGridSettings(const Scene::SceneSettings::GridSettings& grid)
{
    if (!gridSpacingSpin || !gridDivisionsSpin || !gridExtentSpin)
        return;
    gridSpacingSpin->setValue(std::max(0.001, static_cast<double>(grid.majorSpacing)));
    gridDivisionsSpin->setValue(std::max(1, grid.minorDivisions));
    gridExtentSpin->setValue(std::max(1, grid.majorExtent));
}

Scene::SceneSettings::GridSettings ViewSettingsDialog::gridSettings() const
{
    Scene::SceneSettings::GridSettings grid;
    grid.majorSpacing = static_cast<float>(gridSpacingSpin ? gridSpacingSpin->value() : 1.0);
    grid.minorDivisions = gridDivisionsSpin ? gridDivisionsSpin->value() : 4;
    grid.majorExtent = gridExtentSpin ? gridExtentSpin->value() : 50;
    return grid;
}

void ViewSettingsDialog::setShadowSettings(const Scene::SceneSettings::ShadowSettings& shadows)
{
    if (!shadowEnableCheck || !shadowQualityCombo || !shadowStrengthSpin || !shadowBiasSpin)
        return;
    shadowEnableCheck->setChecked(shadows.enabled);
    int qualityIndex = shadowQualityCombo->findData(static_cast<int>(shadows.quality));
    if (qualityIndex < 0)
        qualityIndex = shadowQualityCombo->findData(static_cast<int>(Scene::SceneSettings::ShadowQuality::Medium));
    if (qualityIndex >= 0)
        shadowQualityCombo->setCurrentIndex(qualityIndex);
    shadowStrengthSpin->setValue(static_cast<double>(std::clamp(shadows.strength, 0.0f, 1.0f) * 100.0f));
    shadowBiasSpin->setValue(static_cast<double>(std::max(0.0001f, shadows.bias)));
    shadowQualityCombo->setEnabled(shadows.enabled);
    shadowStrengthSpin->setEnabled(shadows.enabled);
    shadowBiasSpin->setEnabled(shadows.enabled);
}

Scene::SceneSettings::ShadowSettings ViewSettingsDialog::shadowSettings() const
{
    Scene::SceneSettings::ShadowSettings shadows;
    if (shadowEnableCheck)
        shadows.enabled = shadowEnableCheck->isChecked();
    if (shadowQualityCombo) {
        int data = shadowQualityCombo->currentData().toInt();
        data = std::clamp(data, 0, 2);
        shadows.quality = static_cast<Scene::SceneSettings::ShadowQuality>(data);
    }
    if (shadowStrengthSpin)
        shadows.strength = static_cast<float>(shadowStrengthSpin->value() / 100.0);
    if (shadowBiasSpin)
        shadows.bias = static_cast<float>(shadowBiasSpin->value());
    return shadows;
}

