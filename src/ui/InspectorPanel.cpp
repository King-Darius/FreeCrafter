#include "InspectorPanel.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QtMath>

#include <algorithm>

#include "../GeometryKernel/GeometryObject.h"

namespace {
constexpr double kDefaultMinRadius = 0.01;
constexpr double kMaxRadius = 100000.0;
constexpr double kAngleMin = 0.0;
constexpr double kAngleMax = 360.0;
constexpr double kSweepMin = 0.1;
constexpr double kSweepMax = 360.0;

double computeSweepDegrees(const ShapeBuilder::ArcDefinition& definition)
{
    ShapeBuilder::ArcDefinition def = definition;
    auto canonicalize = [](double angle) {
        while (angle < 0.0)
            angle += 360.0;
        while (angle >= 360.0)
            angle -= 360.0;
        return angle;
    };
    double start = qRadiansToDegrees(def.startAngle);
    double end = qRadiansToDegrees(def.endAngle);
    start = canonicalize(start);
    end = canonicalize(end);
    double sweep = def.counterClockwise ? (end - start) : (start - end);
    if (def.counterClockwise) {
        if (sweep < 0.0)
            sweep += 360.0;
    } else {
        if (sweep < 0.0)
            sweep = -sweep;
    }
    return sweep;
}

QDoubleSpinBox* createCoordinateSpinbox(QWidget* parent)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setRange(-kMaxRadius, kMaxRadius);
    spin->setDecimals(3);
    spin->setSingleStep(0.1);
    return spin;
}

} // namespace

InspectorPanel::InspectorPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    titleLabel = new QLabel(tr("Inspector"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() + 1.5);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    stacked = new QStackedWidget(this);
    layout->addWidget(stacked, 1);

    nonePage = createMessagePage(tr("Select a shape to view its properties."), &noneLabel);
    multiPage = createMessagePage(tr("Multiple selection: editing is unavailable."), &multiLabel);
    unsupportedPage = createMessagePage(tr("The selected object has no editable parameters."), &unsupportedLabel);

    stacked->addWidget(nonePage);
    stacked->addWidget(multiPage);
    stacked->addWidget(unsupportedPage);

    circlePage = new QWidget(this);
    auto* circleLayout = new QFormLayout(circlePage);
    circleLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    circleRadius = new QDoubleSpinBox(circlePage);
    circleRadius->setRange(kDefaultMinRadius, kMaxRadius);
    circleRadius->setDecimals(3);
    circleRadius->setSingleStep(0.1);
    circleLayout->addRow(tr("Radius"), circleRadius);
    circleSegments = new QSpinBox(circlePage);
    circleSegments->setRange(8, 512);
    circleSegments->setSingleStep(1);
    circleLayout->addRow(tr("Segments"), circleSegments);
    stacked->addWidget(circlePage);

    polygonPage = new QWidget(this);
    auto* polygonLayout = new QFormLayout(polygonPage);
    polygonLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    polygonRadius = new QDoubleSpinBox(polygonPage);
    polygonRadius->setRange(kDefaultMinRadius, kMaxRadius);
    polygonRadius->setDecimals(3);
    polygonRadius->setSingleStep(0.1);
    polygonLayout->addRow(tr("Radius"), polygonRadius);
    polygonSides = new QSpinBox(polygonPage);
    polygonSides->setRange(3, 128);
    polygonSides->setSingleStep(1);
    polygonLayout->addRow(tr("Sides"), polygonSides);
    stacked->addWidget(polygonPage);

    arcPage = new QWidget(this);
    auto* arcLayout = new QFormLayout(arcPage);
    arcLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    arcRadius = new QDoubleSpinBox(arcPage);
    arcRadius->setRange(kDefaultMinRadius, kMaxRadius);
    arcRadius->setDecimals(3);
    arcRadius->setSingleStep(0.1);
    arcLayout->addRow(tr("Radius"), arcRadius);
    arcStart = new QDoubleSpinBox(arcPage);
    arcStart->setRange(kAngleMin, kAngleMax);
    arcStart->setDecimals(2);
    arcStart->setSingleStep(1.0);
    arcLayout->addRow(tr("Start angle"), arcStart);
    arcSweep = new QDoubleSpinBox(arcPage);
    arcSweep->setRange(kSweepMin, kSweepMax);
    arcSweep->setDecimals(2);
    arcSweep->setSingleStep(1.0);
    arcLayout->addRow(tr("Sweep"), arcSweep);
    arcSegments = new QSpinBox(arcPage);
    arcSegments->setRange(8, 720);
    arcSegments->setSingleStep(1);
    arcLayout->addRow(tr("Segments"), arcSegments);
    arcDirection = new QCheckBox(tr("Counter-clockwise"), arcPage);
    arcLayout->addRow(QString(), arcDirection);
    stacked->addWidget(arcPage);

    bezierPage = new QWidget(this);
    auto* bezierLayout = new QVBoxLayout(bezierPage);
    bezierLayout->setContentsMargins(0, 0, 0, 0);
    bezierLayout->setSpacing(12);
    auto* bezierInfo = new QLabel(tr("Edit Bézier endpoints and handles. Coordinates are measured in model units."), bezierPage);
    bezierInfo->setWordWrap(true);
    bezierLayout->addWidget(bezierInfo);

    auto* grid = new QGridLayout();
    grid->setColumnStretch(4, 1);

    auto addPointRow = [&](const QString& label, int row, PointEditors& editors) {
        auto* name = new QLabel(label, bezierPage);
        grid->addWidget(name, row, 0);
        editors.x = createCoordinateSpinbox(bezierPage);
        grid->addWidget(editors.x, row, 1);
        editors.y = createCoordinateSpinbox(bezierPage);
        grid->addWidget(editors.y, row, 2);
        editors.z = createCoordinateSpinbox(bezierPage);
        grid->addWidget(editors.z, row, 3);
        grid->setRowMinimumHeight(row, 28);
        connectPointEditors(editors);
    };

    addPointRow(tr("Start"), 0, bezierP0);
    addPointRow(tr("Start handle"), 1, bezierH0);
    addPointRow(tr("End handle"), 2, bezierH1);
    addPointRow(tr("End"), 3, bezierP1);

    bezierLayout->addLayout(grid);

    auto* bezierForm = new QFormLayout();
    bezierForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    bezierSegments = new QSpinBox(bezierPage);
    bezierSegments->setRange(8, 720);
    bezierSegments->setSingleStep(1);
    bezierForm->addRow(tr("Segments"), bezierSegments);
    bezierLayout->addLayout(bezierForm);
    bezierLayout->addStretch();

    stacked->addWidget(bezierPage);

    connect(circleRadius, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitCircle);
    connect(circleSegments, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitCircle);

    connect(polygonRadius, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitPolygon);
    connect(polygonSides, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitPolygon);

    connect(arcRadius, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitArc);
    connect(arcStart, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitArc);
    connect(arcSweep, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitArc);
    connect(arcSegments, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitArc);
    connect(arcDirection, &QCheckBox::checkStateChanged, this, &InspectorPanel::commitArc);

    connect(bezierSegments, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitBezier);

    resetState();
}

QWidget* InspectorPanel::createMessagePage(const QString& text, QLabel** labelOut)
{
    QWidget* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->addStretch();
    auto* label = new QLabel(text, page);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addStretch();
    if (labelOut)
        *labelOut = label;
    return page;
}

void InspectorPanel::resetState()
{
    updating = true;
    currentKernel = nullptr;
    currentObject = nullptr;
    currentMetadata = GeometryKernel::ShapeMetadata{};
    titleLabel->setText(tr("Inspector"));
    stacked->setCurrentWidget(nonePage);
    updating = false;
}

void InspectorPanel::updateSelection(GeometryKernel* kernel, const std::vector<GeometryObject*>& selection)
{
    if (updating)
        return;

    if (!kernel || selection.empty()) {
        resetState();
        if (noneLabel)
            noneLabel->setText(tr("Select a shape to view its properties."));
        return;
    }

    if (selection.size() > 1) {
        currentKernel = kernel;
        currentObject = nullptr;
        currentMetadata = GeometryKernel::ShapeMetadata{};
        titleLabel->setText(tr("Multiple selection"));
        if (multiLabel)
            multiLabel->setText(tr("Multiple selection detected. Editing parameters requires a single selection."));
        stacked->setCurrentWidget(multiPage);
        return;
    }

    GeometryObject* candidate = selection.front();
    if (!candidate || candidate->getType() != ObjectType::Curve) {
        currentKernel = kernel;
        currentObject = nullptr;
        currentMetadata = GeometryKernel::ShapeMetadata{};
        titleLabel->setText(tr("No editable parameters"));
        if (unsupportedLabel)
            unsupportedLabel->setText(tr("The selected entity cannot be edited."));
        stacked->setCurrentWidget(unsupportedPage);
        return;
    }

    currentKernel = kernel;
    currentObject = candidate;

    auto metadata = kernel->shapeMetadata(candidate);
    if (!metadata.has_value()) {
        currentMetadata = GeometryKernel::ShapeMetadata{};
        titleLabel->setText(tr("No editable parameters"));
        if (unsupportedLabel)
            unsupportedLabel->setText(tr("This curve was not created by a parametric tool."));
        stacked->setCurrentWidget(unsupportedPage);
        return;
    }

    currentMetadata = *metadata;
    populateFromMetadata();
}

void InspectorPanel::populateFromMetadata()
{
    updating = true;
    switch (currentMetadata.type) {
    case GeometryKernel::ShapeMetadata::Type::Circle:
        titleLabel->setText(tr("Circle"));
        stacked->setCurrentWidget(circlePage);
        populateCircle();
        break;
    case GeometryKernel::ShapeMetadata::Type::Polygon:
        titleLabel->setText(tr("Polygon"));
        stacked->setCurrentWidget(polygonPage);
        populatePolygon();
        break;
    case GeometryKernel::ShapeMetadata::Type::Arc:
        titleLabel->setText(tr("Arc"));
        stacked->setCurrentWidget(arcPage);
        populateArc();
        break;
    case GeometryKernel::ShapeMetadata::Type::Bezier:
        titleLabel->setText(tr("Bézier"));
        stacked->setCurrentWidget(bezierPage);
        populateBezier();
        break;
    default:
        titleLabel->setText(tr("No editable parameters"));
        stacked->setCurrentWidget(unsupportedPage);
        if (unsupportedLabel)
            unsupportedLabel->setText(tr("This curve type is not yet editable."));
        break;
    }
    updating = false;
}

void InspectorPanel::populateCircle()
{
    if (!circleRadius || !circleSegments)
        return;
    const auto& data = currentMetadata.circle;
    {
        QSignalBlocker blocker(circleRadius);
        circleRadius->setValue(data.radius);
    }
    {
        QSignalBlocker blocker(circleSegments);
        circleSegments->setValue(std::max(8, data.segments));
    }
}

void InspectorPanel::populatePolygon()
{
    if (!polygonRadius || !polygonSides)
        return;
    const auto& data = currentMetadata.polygon;
    {
        QSignalBlocker blocker(polygonRadius);
        polygonRadius->setValue(data.radius);
    }
    {
        QSignalBlocker blocker(polygonSides);
        polygonSides->setValue(std::max(3, data.sides));
    }
}

void InspectorPanel::populateArc()
{
    if (!arcRadius || !arcStart || !arcSweep || !arcSegments || !arcDirection)
        return;
    const auto& data = currentMetadata.arc.definition;
    {
        QSignalBlocker blocker(arcRadius);
        arcRadius->setValue(data.radius);
    }
    {
        QSignalBlocker blocker(arcStart);
        arcStart->setValue(qRadiansToDegrees(data.startAngle));
    }
    double sweep = computeSweepDegrees(data);
    {
        QSignalBlocker blocker(arcSweep);
        arcSweep->setValue(std::max(kSweepMin, sweep));
    }
    {
        QSignalBlocker blocker(arcSegments);
        arcSegments->setValue(std::max(8, data.segments));
    }
    {
        QSignalBlocker blocker(arcDirection);
        arcDirection->setChecked(data.counterClockwise);
    }
}

void InspectorPanel::populateBezier()
{
    setPointEditors(currentMetadata);
    if (bezierSegments) {
        QSignalBlocker blocker(bezierSegments);
        int segments = currentMetadata.bezier.definition.segments;
        if (segments < 8)
            segments = 24;
        bezierSegments->setValue(segments);
    }
}

void InspectorPanel::setPointEditors(const GeometryKernel::ShapeMetadata& metadata)
{
    setPointEditors(bezierP0, metadata.bezier.definition.p0);
    setPointEditors(bezierH0, metadata.bezier.definition.h0);
    setPointEditors(bezierH1, metadata.bezier.definition.h1);
    setPointEditors(bezierP1, metadata.bezier.definition.p1);
}

void InspectorPanel::setPointEditors(PointEditors& editors, const Vector3& value)
{
    if (!editors.x || !editors.y || !editors.z)
        return;
    QSignalBlocker bx(editors.x);
    QSignalBlocker by(editors.y);
    QSignalBlocker bz(editors.z);
    editors.x->setValue(value.x);
    editors.y->setValue(value.y);
    editors.z->setValue(value.z);
}

void InspectorPanel::connectPointEditors(PointEditors& editors)
{
    connect(editors.x, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitBezier);
    connect(editors.y, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitBezier);
    connect(editors.z, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitBezier);
}

Vector3 InspectorPanel::pointFromEditors(const PointEditors& editors) const
{
    return Vector3(static_cast<float>(editors.x->value()), static_cast<float>(editors.y->value()),
                   static_cast<float>(editors.z->value()));
}

void InspectorPanel::commitCircle()
{
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    metadata.circle.radius = static_cast<float>(circleRadius->value());
    metadata.circle.segments = circleSegments->value();
    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populateCircle();
        return;
    }
    currentMetadata = *currentKernel->shapeMetadata(currentObject);
    populateCircle();
    emit shapeModified();
}

void InspectorPanel::commitPolygon()
{
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    metadata.polygon.radius = static_cast<float>(polygonRadius->value());
    metadata.polygon.sides = polygonSides->value();
    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populatePolygon();
        return;
    }
    currentMetadata = *currentKernel->shapeMetadata(currentObject);
    populatePolygon();
    emit shapeModified();
}

void InspectorPanel::commitArc()
{
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    auto& def = metadata.arc.definition;
    def.radius = static_cast<float>(arcRadius->value());
    def.startAngle = qDegreesToRadians(static_cast<float>(arcStart->value()));
    double sweepDeg = arcSweep->value();
    double sweepRad = qDegreesToRadians(sweepDeg);
    bool ccw = arcDirection->isChecked();
    def.counterClockwise = ccw;
    if (ccw)
        def.endAngle = static_cast<float>(def.startAngle + sweepRad);
    else
        def.endAngle = static_cast<float>(def.startAngle - sweepRad);
    def.segments = arcSegments->value();
    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populateArc();
        return;
    }
    currentMetadata = *currentKernel->shapeMetadata(currentObject);
    populateArc();
    emit shapeModified();
}

void InspectorPanel::commitBezier()
{
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    metadata.bezier.definition.p0 = pointFromEditors(bezierP0);
    metadata.bezier.definition.h0 = pointFromEditors(bezierH0);
    metadata.bezier.definition.h1 = pointFromEditors(bezierH1);
    metadata.bezier.definition.p1 = pointFromEditors(bezierP1);
    metadata.bezier.definition.segments = bezierSegments->value();
    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populateBezier();
        return;
    }
    currentMetadata = *currentKernel->shapeMetadata(currentObject);
    populateBezier();
    emit shapeModified();
}

