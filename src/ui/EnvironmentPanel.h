#pragma once

#include <QWidget>

#include "SunSettings.h"

class QDateEdit;
class QTimeEdit;
class QSlider;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;

class EnvironmentPanel : public QWidget
{
    Q_OBJECT
public:
    explicit EnvironmentPanel(QWidget* parent = nullptr);

    void setSettings(const SunSettings& settings);

signals:
    void settingsChanged(const SunSettings& settings);

private:
    void syncControls();
    void emitChanges();

    void handleTimeSlider(int minutes);
    void handleTimeChanged(const QTime& time);

    SunSettings current;
    bool updating = false;

    QDateEdit* dateEdit = nullptr;
    QTimeEdit* timeEdit = nullptr;
    QSlider* timeSlider = nullptr;
    QDoubleSpinBox* latitudeSpin = nullptr;
    QDoubleSpinBox* longitudeSpin = nullptr;
    QDoubleSpinBox* timezoneSpin = nullptr;
    QCheckBox* dstCheck = nullptr;
    QCheckBox* shadowsCheck = nullptr;
    QComboBox* qualityCombo = nullptr;
    QSlider* strengthSlider = nullptr;
    QLabel* strengthLabel = nullptr;
    QDoubleSpinBox* biasSpin = nullptr;
};
