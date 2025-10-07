#include "EnvironmentPanel.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QDateEdit>
#include <QTimeEdit>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QVariant>
#include <cmath>
#include <algorithm>

namespace {
int timeToSliderMinutes(const QTime& time)
{
    if (!time.isValid())
        return 12 * 60;
    return time.hour() * 60 + time.minute();
}

QTime sliderToTime(int minutes)
{
    minutes = std::clamp(minutes, 0, 24 * 60 - 1);
    const int hour = minutes / 60;
    const int minute = minutes % 60;
    return QTime(hour, minute, 0);
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

    dateEdit = new QDateEdit(this);
    dateEdit->setCalendarPopup(true);
    sunLayout->addRow(tr("Date"), dateEdit);

    timeEdit = new QTimeEdit(this);
    timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    sunLayout->addRow(tr("Time"), timeEdit);

    timeSlider = new QSlider(Qt::Horizontal, this);
    timeSlider->setRange(0, 24 * 60 - 1);
    sunLayout->addRow(tr("Time Slider"), timeSlider);

    latitudeSpin = new QDoubleSpinBox(this);
    latitudeSpin->setRange(-90.0, 90.0);
    latitudeSpin->setDecimals(4);
    latitudeSpin->setSingleStep(0.1);
    sunLayout->addRow(tr("Latitude"), latitudeSpin);

    longitudeSpin = new QDoubleSpinBox(this);
    longitudeSpin->setRange(-180.0, 180.0);
    longitudeSpin->setDecimals(4);
    longitudeSpin->setSingleStep(0.1);
    sunLayout->addRow(tr("Longitude"), longitudeSpin);

    timezoneSpin = new QDoubleSpinBox(this);
    timezoneSpin->setRange(-12.0, 14.0);
    timezoneSpin->setDecimals(2);
    timezoneSpin->setSingleStep(0.5);
    sunLayout->addRow(tr("UTC Offset"), timezoneSpin);

    dstCheck = new QCheckBox(tr("Daylight Saving (+1h)"), this);
    sunLayout->addRow(QString(), dstCheck);

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

    connect(dateEdit, &QDateEdit::dateChanged, this, [this](const QDate& date) {
        current.date = date;
        emitChanges();
    });
    connect(timeEdit, &QTimeEdit::timeChanged, this, &EnvironmentPanel::handleTimeChanged);
    connect(timeSlider, &QSlider::valueChanged, this, &EnvironmentPanel::handleTimeSlider);
    connect(latitudeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        current.latitude = value;
        emitChanges();
    });
    connect(longitudeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        current.longitude = value;
        emitChanges();
    });
    connect(timezoneSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        current.timezoneMinutes = static_cast<int>(std::round(value * 60.0));
        emitChanges();
    });
    connect(dstCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        current.daylightSaving = enabled;
        emitChanges();
    });
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
    dateEdit->setDate(current.date);
    timeEdit->setTime(current.time.isValid() ? current.time : QTime(12, 0, 0));
    timeSlider->setValue(timeToSliderMinutes(timeEdit->time()));
    latitudeSpin->setValue(current.latitude);
    longitudeSpin->setValue(current.longitude);
    timezoneSpin->setValue(static_cast<double>(current.timezoneMinutes) / 60.0);
    dstCheck->setChecked(current.daylightSaving);
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

void EnvironmentPanel::handleTimeSlider(int minutes)
{
    if (updating)
        return;
    updating = true;
    const QTime newTime = sliderToTime(minutes);
    timeEdit->setTime(newTime);
    updating = false;
    current.time = newTime;
    emit settingsChanged(current);
}

void EnvironmentPanel::handleTimeChanged(const QTime& time)
{
    if (updating)
        return;
    updating = true;
    const int minutes = timeToSliderMinutes(time);
    timeSlider->setValue(minutes);
    updating = false;
    current.time = time;
    emit settingsChanged(current);
}
