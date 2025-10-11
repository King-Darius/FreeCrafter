#include "EnvironmentPanel.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QVariant>

#include <algorithm>
#include <cmath>

namespace {

constexpr float kElevationMin = -10.0f;
constexpr float kElevationMax = 90.0f;
constexpr int kAngleMultiplier = 10; // 0.1° increments

int elevationToSlider(float degrees)
{
    float clamped = std::clamp(degrees, kElevationMin, kElevationMax);
    return static_cast<int>(std::round((clamped - kElevationMin) * kAngleMultiplier));
}

float sliderToElevation(int sliderValue)
{
    float value = static_cast<float>(sliderValue) / kAngleMultiplier + kElevationMin;
    return std::clamp(value, kElevationMin, kElevationMax);
}

int azimuthToSlider(float degrees)
{
    float normalized = std::fmod(degrees, 360.0f);
    if (normalized < 0.0f)
        normalized += 360.0f;
    return static_cast<int>(std::round(normalized * kAngleMultiplier));
}

float sliderToAzimuth(int sliderValue)
{
    float degrees = static_cast<float>(sliderValue) / kAngleMultiplier;
    while (degrees < 0.0f)
        degrees += 360.0f;
    while (degrees >= 360.0f)
        degrees -= 360.0f;
    return degrees;
}

QString qualityLabel(SunSettings::ShadowQuality quality)
{
    switch (quality) {
    case SunSettings::ShadowQuality::Low:
        return QObject::tr("Low (512)");
    case SunSettings::ShadowQuality::Medium:
        return QObject::tr("Medium (1024)");
    case SunSettings::ShadowQuality::High:
        return QObject::tr("High (2048)");
    }
    return QObject::tr("Custom");
}

} // namespace

EnvironmentPanel::EnvironmentPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* sunGroup = new QGroupBox(tr("Sun Position"), this);
    auto* sunLayout = new QFormLayout(sunGroup);
    sunLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    elevationSlider = new QSlider(Qt::Horizontal, this);
    elevationSlider->setRange(0, elevationToSlider(kElevationMax));
    elevationSpin = new QDoubleSpinBox(this);
    elevationSpin->setRange(kElevationMin, kElevationMax);
    elevationSpin->setDecimals(1);
    elevationSpin->setSingleStep(0.5);

    auto* elevationContainer = new QWidget(this);
    auto* elevationLayout = new QHBoxLayout(elevationContainer);
    elevationLayout->setContentsMargins(0, 0, 0, 0);
    elevationLayout->setSpacing(6);
    elevationLayout->addWidget(elevationSlider, 1);
    elevationLayout->addWidget(elevationSpin, 0);
    sunLayout->addRow(tr("Elevation"), elevationContainer);

    azimuthSlider = new QSlider(Qt::Horizontal, this);
    azimuthSlider->setRange(0, azimuthToSlider(360.0f));
    azimuthSpin = new QDoubleSpinBox(this);
    azimuthSpin->setRange(0.0, 360.0);
    azimuthSpin->setDecimals(1);
    azimuthSpin->setSingleStep(1.0);

    auto* azimuthContainer = new QWidget(this);
    auto* azimuthLayout = new QHBoxLayout(azimuthContainer);
    azimuthLayout->setContentsMargins(0, 0, 0, 0);
    azimuthLayout->setSpacing(6);
    azimuthLayout->addWidget(azimuthSlider, 1);
    azimuthLayout->addWidget(azimuthSpin, 0);
    sunLayout->addRow(tr("Azimuth"), azimuthContainer);

    layout->addWidget(sunGroup);

    auto* shadowGroup = new QGroupBox(tr("Shadows"), this);
    auto* shadowLayout = new QFormLayout(shadowGroup);
    shadowLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    shadowsCheck = new QCheckBox(tr("Cast Shadows"), this);
    shadowLayout->addRow(QString(), shadowsCheck);

    qualityCombo = new QComboBox(this);
    qualityCombo->addItem(qualityLabel(SunSettings::ShadowQuality::Low), static_cast<int>(SunSettings::ShadowQuality::Low));
    qualityCombo->addItem(qualityLabel(SunSettings::ShadowQuality::Medium), static_cast<int>(SunSettings::ShadowQuality::Medium));
    qualityCombo->addItem(qualityLabel(SunSettings::ShadowQuality::High), static_cast<int>(SunSettings::ShadowQuality::High));
    shadowLayout->addRow(tr("Quality"), qualityCombo);

    strengthSlider = new QSlider(Qt::Horizontal, this);
    strengthSlider->setRange(0, 100);
    strengthLabel = new QLabel(this);
    strengthLabel->setMinimumWidth(48);
    auto* strengthContainer = new QWidget(this);
    auto* strengthLayout = new QHBoxLayout(strengthContainer);
    strengthLayout->setContentsMargins(0, 0, 0, 0);
    strengthLayout->setSpacing(6);
    strengthLayout->addWidget(strengthSlider, 1);
    strengthLayout->addWidget(strengthLabel, 0);
    shadowLayout->addRow(tr("Darkness"), strengthContainer);

    biasSpin = new QDoubleSpinBox(this);
    biasSpin->setRange(0.0001, 0.02);
    biasSpin->setDecimals(4);
    biasSpin->setSingleStep(0.0005);
    shadowLayout->addRow(tr("Bias"), biasSpin);

    layout->addWidget(shadowGroup);
    layout->addStretch(1);

    connect(elevationSlider, &QSlider::valueChanged, this, &EnvironmentPanel::handleElevationSlider);
    connect(elevationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EnvironmentPanel::handleElevationSpin);
    connect(azimuthSlider, &QSlider::valueChanged, this, &EnvironmentPanel::handleAzimuthSlider);
    connect(azimuthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EnvironmentPanel::handleAzimuthSpin);
    connect(shadowsCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        current.shadowsEnabled = enabled;
        emitChanges();
    });
    connect(qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QVariant data = qualityCombo->itemData(index);
        if (data.isValid()) {
            current.shadowQuality = static_cast<SunSettings::ShadowQuality>(data.toInt());
            emitChanges();
        }
    });
    connect(strengthSlider, &QSlider::valueChanged, this, [this](int value) {
        if (strengthLabel)
            strengthLabel->setText(tr("%1%").arg(value));
        current.shadowStrength = static_cast<float>(value) / 100.0f;
        emitChanges();
    });
    connect(biasSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        current.shadowBias = static_cast<float>(value);
        emitChanges();
    });

    syncControls();
}

void EnvironmentPanel::setSettings(const SunSettings& settings)
{
    current = settings;
    syncControls();
}

void EnvironmentPanel::syncControls()
{
    updating = true;
    elevationSlider->setValue(elevationToSlider(current.elevationDegrees));
    elevationSpin->setValue(current.elevationDegrees);
    azimuthSlider->setValue(azimuthToSlider(current.azimuthDegrees));
    azimuthSpin->setValue(current.azimuthDegrees);
    shadowsCheck->setChecked(current.shadowsEnabled);

    int qualityIndex = qualityCombo->findData(static_cast<int>(current.shadowQuality));
    if (qualityIndex >= 0)
        qualityCombo->setCurrentIndex(qualityIndex);

    int strengthValue = static_cast<int>(std::round(current.shadowStrength * 100.0f));
    strengthSlider->setValue(strengthValue);
    if (strengthLabel)
        strengthLabel->setText(tr("%1%").arg(strengthValue));
    biasSpin->setValue(current.shadowBias);
    updating = false;
}

void EnvironmentPanel::emitChanges()
{
    if (updating)
        return;
    emit settingsChanged(current);
}

void EnvironmentPanel::handleElevationSlider(int sliderValue)
{
    if (updating)
        return;
    updating = true;
    float degrees = sliderToElevation(sliderValue);
    elevationSpin->setValue(degrees);
    updating = false;
    current.elevationDegrees = degrees;
    emitChanges();
}

void EnvironmentPanel::handleElevationSpin(double degrees)
{
    if (updating)
        return;
    updating = true;
    elevationSlider->setValue(elevationToSlider(static_cast<float>(degrees)));
    updating = false;
    current.elevationDegrees = static_cast<float>(degrees);
    emitChanges();
}

void EnvironmentPanel::handleAzimuthSlider(int sliderValue)
{
    if (updating)
        return;
    updating = true;
    float degrees = sliderToAzimuth(sliderValue);
    azimuthSpin->setValue(degrees);
    updating = false;
    current.azimuthDegrees = degrees;
    emitChanges();
}

void EnvironmentPanel::handleAzimuthSpin(double degrees)
{
    if (updating)
        return;
    updating = true;
    azimuthSlider->setValue(azimuthToSlider(static_cast<float>(degrees)));
    updating = false;
    current.azimuthDegrees = static_cast<float>(degrees);
    emitChanges();
}

