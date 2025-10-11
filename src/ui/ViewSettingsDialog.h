#pragma once

#include <QDialog>

#include "CameraController.h"
#include "Scene/SceneSettings.h"

class QDoubleSpinBox;
class QRadioButton;
class QCheckBox;
class QComboBox;
class QSpinBox;

class ViewSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ViewSettingsDialog(QWidget* parent = nullptr);

    void setFieldOfView(float fovDegrees);
    float fieldOfView() const;

    void setProjectionMode(CameraController::ProjectionMode mode);
    CameraController::ProjectionMode projectionMode() const;

    void setGridSettings(const Scene::SceneSettings::GridSettings& grid);
    Scene::SceneSettings::GridSettings gridSettings() const;

    void setShadowSettings(const Scene::SceneSettings::ShadowSettings& shadows);
    Scene::SceneSettings::ShadowSettings shadowSettings() const;

private:
    QDoubleSpinBox* fovSpin = nullptr;
    QRadioButton* perspectiveButton = nullptr;
    QRadioButton* parallelButton = nullptr;
    QDoubleSpinBox* gridSpacingSpin = nullptr;
    QSpinBox* gridDivisionsSpin = nullptr;
    QSpinBox* gridExtentSpin = nullptr;
    QCheckBox* shadowEnableCheck = nullptr;
    QComboBox* shadowQualityCombo = nullptr;
    QDoubleSpinBox* shadowStrengthSpin = nullptr;
    QDoubleSpinBox* shadowBiasSpin = nullptr;
};

