#pragma once

#include <QDialog>

#include "CameraController.h"

class QDoubleSpinBox;
class QRadioButton;

class ViewSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ViewSettingsDialog(QWidget* parent = nullptr);

    void setFieldOfView(float fovDegrees);
    float fieldOfView() const;

    void setProjectionMode(CameraController::ProjectionMode mode);
    CameraController::ProjectionMode projectionMode() const;

private:
    QDoubleSpinBox* fovSpin = nullptr;
    QRadioButton* perspectiveButton = nullptr;
    QRadioButton* parallelButton = nullptr;
};

