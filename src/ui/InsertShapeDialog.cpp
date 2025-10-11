#include "InsertShapeDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {
constexpr double kMinimumSize = 0.01;
constexpr double kMaximumSize = 100000.0;
}

InsertShapeDialog::InsertShapeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Insert Shapes"));
    setModal(true);

    typeCombo = new QComboBox(this);
    typeCombo->addItem(tr("Box"), static_cast<int>(Scene::PrimitiveType::Box));
    typeCombo->addItem(tr("Plane"), static_cast<int>(Scene::PrimitiveType::Plane));
    typeCombo->addItem(tr("Cylinder"), static_cast<int>(Scene::PrimitiveType::Cylinder));
    typeCombo->addItem(tr("Sphere"), static_cast<int>(Scene::PrimitiveType::Sphere));
    typeCombo->setToolTip(tr("Choose which primitive to add."));

    stacked = new QStackedWidget(this);

    auto* boxPage = new QWidget(this);
    auto* boxLayout = new QFormLayout(boxPage);
    boxWidth = new QDoubleSpinBox(boxPage);
    boxWidth->setRange(kMinimumSize, kMaximumSize);
    boxWidth->setDecimals(3);
    boxWidth->setValue(1.0);
    boxWidth->setSuffix(tr(" mm"));
    boxWidth->setToolTip(tr("Width of the box along X."));
    boxDepth = new QDoubleSpinBox(boxPage);
    boxDepth->setRange(kMinimumSize, kMaximumSize);
    boxDepth->setDecimals(3);
    boxDepth->setValue(1.0);
    boxDepth->setSuffix(tr(" mm"));
    boxDepth->setToolTip(tr("Depth of the box along Z."));
    boxHeight = new QDoubleSpinBox(boxPage);
    boxHeight->setRange(kMinimumSize, kMaximumSize);
    boxHeight->setDecimals(3);
    boxHeight->setValue(1.0);
    boxHeight->setSuffix(tr(" mm"));
    boxHeight->setToolTip(tr("Height of the box along Y."));
    boxLayout->addRow(tr("Width"), boxWidth);
    boxLayout->addRow(tr("Depth"), boxDepth);
    boxLayout->addRow(tr("Height"), boxHeight);
    stacked->addWidget(boxPage);

    auto* planePage = new QWidget(this);
    auto* planeLayout = new QFormLayout(planePage);
    planeWidth = new QDoubleSpinBox(planePage);
    planeWidth->setRange(kMinimumSize, kMaximumSize);
    planeWidth->setDecimals(3);
    planeWidth->setValue(2.0);
    planeWidth->setSuffix(tr(" mm"));
    planeWidth->setToolTip(tr("Width of the plane along X."));
    planeDepth = new QDoubleSpinBox(planePage);
    planeDepth->setRange(kMinimumSize, kMaximumSize);
    planeDepth->setDecimals(3);
    planeDepth->setValue(2.0);
    planeDepth->setSuffix(tr(" mm"));
    planeDepth->setToolTip(tr("Depth of the plane along Z."));
    planeLayout->addRow(tr("Width"), planeWidth);
    planeLayout->addRow(tr("Depth"), planeDepth);
    stacked->addWidget(planePage);

    auto* cylinderPage = new QWidget(this);
    auto* cylinderLayout = new QFormLayout(cylinderPage);
    cylinderRadius = new QDoubleSpinBox(cylinderPage);
    cylinderRadius->setRange(kMinimumSize, kMaximumSize);
    cylinderRadius->setDecimals(3);
    cylinderRadius->setValue(0.5);
    cylinderRadius->setSuffix(tr(" mm"));
    cylinderRadius->setToolTip(tr("Cylinder radius."));
    cylinderHeight = new QDoubleSpinBox(cylinderPage);
    cylinderHeight->setRange(kMinimumSize, kMaximumSize);
    cylinderHeight->setDecimals(3);
    cylinderHeight->setValue(2.0);
    cylinderHeight->setSuffix(tr(" mm"));
    cylinderHeight->setToolTip(tr("Cylinder height."));
    cylinderSegments = new QSpinBox(cylinderPage);
    cylinderSegments->setRange(8, 256);
    cylinderSegments->setValue(32);
    cylinderSegments->setToolTip(tr("Number of radial segments."));
    cylinderLayout->addRow(tr("Radius"), cylinderRadius);
    cylinderLayout->addRow(tr("Height"), cylinderHeight);
    cylinderLayout->addRow(tr("Segments"), cylinderSegments);
    stacked->addWidget(cylinderPage);

    auto* spherePage = new QWidget(this);
    auto* sphereLayout = new QFormLayout(spherePage);
    sphereRadius = new QDoubleSpinBox(spherePage);
    sphereRadius->setRange(kMinimumSize, kMaximumSize);
    sphereRadius->setDecimals(3);
    sphereRadius->setValue(1.0);
    sphereRadius->setSuffix(tr(" mm"));
    sphereRadius->setToolTip(tr("Sphere radius."));
    sphereSegments = new QSpinBox(spherePage);
    sphereSegments->setRange(8, 256);
    sphereSegments->setValue(32);
    sphereSegments->setToolTip(tr("Number of segments around the equator."));
    sphereRings = new QSpinBox(spherePage);
    sphereRings->setRange(4, 128);
    sphereRings->setValue(16);
    sphereRings->setToolTip(tr("Number of rings from pole to pole."));
    sphereLayout->addRow(tr("Radius"), sphereRadius);
    sphereLayout->addRow(tr("Segments"), sphereSegments);
    sphereLayout->addRow(tr("Rings"), sphereRings);
    stacked->addWidget(spherePage);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(typeCombo);
    layout->addWidget(stacked);
    layout->addWidget(buttonBox);

    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InsertShapeDialog::handleTypeChanged);
    handleTypeChanged(0);
}

Scene::PrimitiveOptions InsertShapeDialog::selectedOptions() const
{
    Scene::PrimitiveType type = static_cast<Scene::PrimitiveType>(typeCombo->currentData().toInt());
    switch (type) {
    case Scene::PrimitiveType::Box:
        return buildBoxOptions();
    case Scene::PrimitiveType::Plane:
        return buildPlaneOptions();
    case Scene::PrimitiveType::Cylinder:
        return buildCylinderOptions();
    case Scene::PrimitiveType::Sphere:
        return buildSphereOptions();
    }
    Scene::PrimitiveOptions options;
    options.type = Scene::PrimitiveType::Box;
    return options;
}

void InsertShapeDialog::handleTypeChanged(int index)
{
    Q_UNUSED(index);
    stacked->setCurrentIndex(typeCombo->currentIndex());
}

Scene::PrimitiveOptions InsertShapeDialog::buildBoxOptions() const
{
    Scene::PrimitiveOptions options;
    options.type = Scene::PrimitiveType::Box;
    options.width = static_cast<float>(boxWidth->value());
    options.depth = static_cast<float>(boxDepth->value());
    options.height = static_cast<float>(boxHeight->value());
    return options;
}

Scene::PrimitiveOptions InsertShapeDialog::buildPlaneOptions() const
{
    Scene::PrimitiveOptions options;
    options.type = Scene::PrimitiveType::Plane;
    options.width = static_cast<float>(planeWidth->value());
    options.depth = static_cast<float>(planeDepth->value());
    options.height = 0.01f;
    return options;
}

Scene::PrimitiveOptions InsertShapeDialog::buildCylinderOptions() const
{
    Scene::PrimitiveOptions options;
    options.type = Scene::PrimitiveType::Cylinder;
    options.radius = static_cast<float>(cylinderRadius->value());
    options.height = static_cast<float>(cylinderHeight->value());
    options.segments = cylinderSegments->value();
    return options;
}

Scene::PrimitiveOptions InsertShapeDialog::buildSphereOptions() const
{
    Scene::PrimitiveOptions options;
    options.type = Scene::PrimitiveType::Sphere;
    options.radius = static_cast<float>(sphereRadius->value());
    options.segments = sphereSegments->value();
    options.rings = sphereRings->value();
    return options;
}

