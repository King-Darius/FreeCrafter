#pragma once

#include <QWidget>

#include "SunSettings.h"

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

    void handleElevationSlider(int sliderValue);
    void handleElevationSpin(double degrees);
    void handleAzimuthSlider(int sliderValue);
    void handleAzimuthSpin(double degrees);

    SunSettings current;
    bool updating = false;

    QSlider* elevationSlider = nullptr;
    QDoubleSpinBox* elevationSpin = nullptr;
    QSlider* azimuthSlider = nullptr;
    QDoubleSpinBox* azimuthSpin = nullptr;
    QCheckBox* shadowsCheck = nullptr;
    QComboBox* qualityCombo = nullptr;
    QSlider* strengthSlider = nullptr;
    QLabel* strengthLabel = nullptr;
    QDoubleSpinBox* biasSpin = nullptr;
};
