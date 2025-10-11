#pragma once

#include "Scene/PrimitiveBuilder.h"

#include <QDialog>

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QStackedWidget;

class InsertShapeDialog : public QDialog {
    Q_OBJECT
public:
    explicit InsertShapeDialog(QWidget* parent = nullptr);

    Scene::PrimitiveOptions selectedOptions() const;

private slots:
    void handleTypeChanged(int index);

private:
    Scene::PrimitiveOptions buildBoxOptions() const;
    Scene::PrimitiveOptions buildPlaneOptions() const;
    Scene::PrimitiveOptions buildCylinderOptions() const;
    Scene::PrimitiveOptions buildSphereOptions() const;

    QComboBox* typeCombo = nullptr;
    QStackedWidget* stacked = nullptr;

    // Box controls
    QDoubleSpinBox* boxWidth = nullptr;
    QDoubleSpinBox* boxDepth = nullptr;
    QDoubleSpinBox* boxHeight = nullptr;

    // Plane controls
    QDoubleSpinBox* planeWidth = nullptr;
    QDoubleSpinBox* planeDepth = nullptr;

    // Cylinder controls
    QDoubleSpinBox* cylinderRadius = nullptr;
    QDoubleSpinBox* cylinderHeight = nullptr;
    QSpinBox* cylinderSegments = nullptr;

    // Sphere controls
    QDoubleSpinBox* sphereRadius = nullptr;
    QSpinBox* sphereSegments = nullptr;
    QSpinBox* sphereRings = nullptr;
};

