#include "InspectorPanel.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVariant>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <set>
#include <unordered_set>

#include "../Core/CommandStack.h"
#include "../GeometryKernel/GeometryObject.h"
#include "../Scene/Document.h"
#include "../Scene/SceneGraphCommands.h"
#include "../Tools/ToolCommands.h"
#include "../Tools/ToolGeometryUtils.h"

namespace {
constexpr double kDefaultMinRadius = 0.01;
constexpr double kMaxRadius = 100000.0;
constexpr double kAngleMin = 0.0;
constexpr double kAngleMax = 360.0;
constexpr double kSweepMin = 0.1;
constexpr double kSweepMax = 360.0;
constexpr double kScaleMin = 0.01;
constexpr double kScaleMax = 1000.0;

QDoubleSpinBox* createCoordinateSpinbox(QWidget* parent)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setRange(-kMaxRadius, kMaxRadius);
    spin->setDecimals(3);
    spin->setSingleStep(0.1);
    return spin;
}

QDoubleSpinBox* createAngleSpinbox(QWidget* parent)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setRange(-kAngleMax, kAngleMax);
    spin->setDecimals(2);
    spin->setSingleStep(1.0);
    spin->setSuffix(QString::fromUtf8("°"));
    return spin;
}

QDoubleSpinBox* createScaleSpinbox(QWidget* parent)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setRange(kScaleMin, kScaleMax);
    spin->setDecimals(3);
    spin->setSingleStep(0.05);
    return spin;
}

QString mixedPlaceholder()
{
    return QObject::tr("--");
}

Vector3 toVector3(const BoundingBox& box)
{
    return Vector3((box.min.x + box.max.x) * 0.5f,
                   (box.min.y + box.max.y) * 0.5f,
                   (box.min.z + box.max.z) * 0.5f);
}

double toDegrees(double radians)
{
    return qRadiansToDegrees(radians);
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

    mixedValueDisplay = tr("--");

    generalSection = new QWidget(this);
    generalSection->setVisible(false);
    generalForm = new QFormLayout(generalSection);
    generalForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    generalForm->setHorizontalSpacing(12);
    generalForm->setVerticalSpacing(6);

    nameEdit = new QLineEdit(generalSection);
    nameEdit->setObjectName(QStringLiteral("inspectorNameEdit"));
    nameEdit->setClearButtonEnabled(true);
    connect(nameEdit, &QLineEdit::editingFinished, this, &InspectorPanel::commitName);
    generalForm->addRow(tr("Name"), nameEdit);

    tagLabel = new QLabel(tr("<None>"), generalSection);
    tagLabel->setObjectName(QStringLiteral("inspectorTagLabel"));
    generalForm->addRow(tr("Tags"), tagLabel);

    visibleCheck = new QCheckBox(tr("Visible"), generalSection);
    visibleCheck->setObjectName(QStringLiteral("inspectorVisibleCheck"));
    connect(visibleCheck, &QCheckBox::stateChanged, this, &InspectorPanel::commitVisibility);
    generalForm->addRow(tr("Visibility"), visibleCheck);

    lockCheck = new QCheckBox(tr("Locked"), generalSection);
    lockCheck->setObjectName(QStringLiteral("inspectorLockCheck"));
    connect(lockCheck, &QCheckBox::stateChanged, this, &InspectorPanel::commitLock);
    generalForm->addRow(tr("Lock"), lockCheck);

    materialCombo = new QComboBox(generalSection);
    materialCombo->setEditable(true);
    materialCombo->setInsertPolicy(QComboBox::NoInsert);
    materialCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    materialCombo->setObjectName(QStringLiteral("inspectorMaterialCombo"));
    if (QLineEdit* comboEdit = materialCombo->lineEdit()) {
        comboEdit->setClearButtonEnabled(true);
        comboEdit->setPlaceholderText(tr("Material"));
        connect(comboEdit, &QLineEdit::editingFinished, this, [this]() {
            commitMaterial(materialCombo->currentText());
        });
    }
    connect(materialCombo, &QComboBox::textActivated, this, &InspectorPanel::commitMaterial);
    generalForm->addRow(tr("Material"), materialCombo);

    layout->addWidget(new CollapsibleSection(tr("Entity"), generalSection, true, this));

    transformSection = new QWidget(this);
    transformSection->setVisible(false);
    auto* transformLayout = new QFormLayout(transformSection);
    transformLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    transformLayout->setHorizontalSpacing(12);
    transformLayout->setVerticalSpacing(6);

    auto createVectorRow = [&](const QString& label, VectorEditors& editors, const QString& baseName, bool editable) {
        QWidget* row = new QWidget(transformSection);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(6);
        auto makeField = [&](const QString& suffix) {
            auto* edit = new QLineEdit(row);
            edit->setAlignment(Qt::AlignRight);
            edit->setPlaceholderText(mixedValueDisplay);
            edit->setObjectName(baseName + suffix);
            if (editable) {
                edit->setClearButtonEnabled(true);
            } else {
                auto* validator = new QDoubleValidator(-kMaxRadius, kMaxRadius, 3, edit);
                validator->setNotation(QDoubleValidator::StandardNotation);
                edit->setValidator(validator);
            }
            edit->setEnabled(editable);
            rowLayout->addWidget(edit);
            return edit;
        };
        editors.x = makeField(QStringLiteral("X"));
        editors.y = makeField(QStringLiteral("Y"));
        editors.z = makeField(QStringLiteral("Z"));
        if (editable) {
            connect(editors.x, &QLineEdit::editingFinished, this, &InspectorPanel::commitPositionX);
            connect(editors.y, &QLineEdit::editingFinished, this, &InspectorPanel::commitPositionY);
            connect(editors.z, &QLineEdit::editingFinished, this, &InspectorPanel::commitPositionZ);
        } else {
            editors.x->setReadOnly(true);
            editors.y->setReadOnly(true);
            editors.z->setReadOnly(true);
        }
        transformLayout->addRow(label, row);
    };

    createVectorRow(tr("Position"), positionEditors, QStringLiteral("inspectorPosition"), true);
    createVectorRow(tr("Rotation"), rotationEditors, QStringLiteral("inspectorRotation"), false);
    createVectorRow(tr("Scale"), scaleEditors, QStringLiteral("inspectorScale"), false);

    layout->addWidget(new CollapsibleSection(tr("Transform"), transformSection, true, this));

    stacked = new QStackedWidget(this);
    layout->addWidget(stacked, 1);

    nonePage = createMessagePage(tr("Select a shape to view its properties."), &noneLabel);
    stacked->addWidget(nonePage);

    detailsPage = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsPage);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(12);

    generalSection = new QWidget(detailsPage);
    auto* generalLayout = new QFormLayout(generalSection);
    generalLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    nameEdit = new QLineEdit(generalSection);
    nameEdit->setPlaceholderText(mixedPlaceholder());
    generalLayout->addRow(tr("Name"), nameEdit);

    visibleCheck = new QCheckBox(tr("Visible"), generalSection);
    visibleCheck->setTristate(true);
    generalLayout->addRow(tr("Visibility"), visibleCheck);

    tagList = new QListWidget(generalSection);
    tagList->setSelectionMode(QAbstractItemView::NoSelection);
    tagList->setUniformItemSizes(true);
    tagList->setMinimumHeight(120);
    generalLayout->addRow(tr("Tags"), tagList);

    materialCombo = new QComboBox(generalSection);
    materialCombo->setEditable(false);
    generalLayout->addRow(tr("Material"), materialCombo);

    auto* transformGroup = new QGroupBox(tr("Transform"), generalSection);
    auto* transformLayout = new QGridLayout(transformGroup);
    transformLayout->setContentsMargins(8, 12, 8, 12);
    transformLayout->setHorizontalSpacing(6);
    transformLayout->setVerticalSpacing(6);

    auto* positionLabel = new QLabel(tr("Position"), transformGroup);
    transformLayout->addWidget(positionLabel, 0, 0);
    positionX = createCoordinateSpinbox(transformGroup);
    positionY = createCoordinateSpinbox(transformGroup);
    positionZ = createCoordinateSpinbox(transformGroup);
    transformLayout->addWidget(positionX, 0, 1);
    transformLayout->addWidget(positionY, 0, 2);
    transformLayout->addWidget(positionZ, 0, 3);

    auto* rotationLabel = new QLabel(tr("Rotation Δ"), transformGroup);
    transformLayout->addWidget(rotationLabel, 1, 0);
    rotationH = createAngleSpinbox(transformGroup);
    rotationP = createAngleSpinbox(transformGroup);
    rotationB = createAngleSpinbox(transformGroup);
    rotationH->setToolTip(tr("Heading (yaw) around the vertical axis."));
    rotationP->setToolTip(tr("Pitch around the X axis."));
    rotationB->setToolTip(tr("Bank (roll) around the Z axis."));
    transformLayout->addWidget(rotationH, 1, 1);
    transformLayout->addWidget(rotationP, 1, 2);
    transformLayout->addWidget(rotationB, 1, 3);

    auto* scaleLabel = new QLabel(tr("Scale ×"), transformGroup);
    transformLayout->addWidget(scaleLabel, 2, 0);
    scaleX = createScaleSpinbox(transformGroup);
    scaleY = createScaleSpinbox(transformGroup);
    scaleZ = createScaleSpinbox(transformGroup);
    scaleX->setValue(1.0);
    scaleY->setValue(1.0);
    scaleZ->setValue(1.0);
    transformLayout->addWidget(scaleX, 2, 1);
    transformLayout->addWidget(scaleY, 2, 2);
    transformLayout->addWidget(scaleZ, 2, 3);

    generalLayout->addRow(transformGroup);
    detailsLayout->addWidget(generalSection);

    typeStack = new QStackedWidget(detailsPage);
    typeNonePage = new QWidget(typeStack);
    {
        auto* messageLayout = new QVBoxLayout(typeNonePage);
        messageLayout->setContentsMargins(16, 16, 16, 16);
        messageLayout->addStretch();
        typeNoneLabel = new QLabel(tr("No additional parameters for the current selection."), typeNonePage);
        typeNoneLabel->setWordWrap(true);
        typeNoneLabel->setAlignment(Qt::AlignCenter);
        messageLayout->addWidget(typeNoneLabel);
        messageLayout->addStretch();
    }
    typeStack->addWidget(typeNonePage);

    circlePage = new QWidget(typeStack);
    auto* circleLayout = new QFormLayout(circlePage);
    circleLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    circleRadius = new QDoubleSpinBox(circlePage);
    circleRadius->setRange(kDefaultMinRadius, kMaxRadius);
    circleRadius->setDecimals(3);
    circleRadius->setSingleStep(0.1);
    circleRadius->setObjectName(QStringLiteral("inspectorCircleRadius"));
    circleLayout->addRow(tr("Radius"), circleRadius);
    circleSegments = new QSpinBox(circlePage);
    circleSegments->setRange(8, 512);
    circleSegments->setSingleStep(1);
    circleSegments->setObjectName(QStringLiteral("inspectorCircleSegments"));
    circleLayout->addRow(tr("Segments"), circleSegments);
    typeStack->addWidget(circlePage);

    polygonPage = new QWidget(typeStack);
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
    typeStack->addWidget(polygonPage);

    arcPage = new QWidget(typeStack);
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
    typeStack->addWidget(arcPage);

    bezierPage = new QWidget(typeStack);
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

    typeStack->addWidget(bezierPage);
    detailsLayout->addWidget(typeStack, 1);

    stacked->addWidget(detailsPage);

    connect(circleRadius, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitCircle);
    connect(circleSegments, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitCircle);

    connect(polygonRadius, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitPolygon);
    connect(polygonSides, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitPolygon);

    connect(arcRadius, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitArc);
    connect(arcStart, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitArc);
    connect(arcSweep, &QDoubleSpinBox::editingFinished, this, &InspectorPanel::commitArc);
    connect(arcSegments, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitArc);
    connect(arcDirection, &QCheckBox::stateChanged, this, &InspectorPanel::commitArc);

    connect(bezierSegments, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &InspectorPanel::commitBezier);

    connect(nameEdit, &QLineEdit::editingFinished, this, &InspectorPanel::commitName);
    connect(visibleCheck, &QCheckBox::stateChanged, this, [this](int state) {
        commitVisibility(static_cast<Qt::CheckState>(state));
    });
    connect(tagList, &QListWidget::itemChanged, this, &InspectorPanel::commitTagChange);
    connect(materialCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &InspectorPanel::commitMaterial);

    connect(positionX, &QDoubleSpinBox::editingFinished, this, [this]() { commitPosition(0); });
    connect(positionY, &QDoubleSpinBox::editingFinished, this, [this]() { commitPosition(1); });
    connect(positionZ, &QDoubleSpinBox::editingFinished, this, [this]() { commitPosition(2); });

    connect(rotationH, &QDoubleSpinBox::editingFinished, this, [this]() { commitRotation(0); });
    connect(rotationP, &QDoubleSpinBox::editingFinished, this, [this]() { commitRotation(1); });
    connect(rotationB, &QDoubleSpinBox::editingFinished, this, [this]() { commitRotation(2); });

    connect(scaleX, &QDoubleSpinBox::editingFinished, this, [this]() { commitScale(0); });
    connect(scaleY, &QDoubleSpinBox::editingFinished, this, [this]() { commitScale(1); });
    connect(scaleZ, &QDoubleSpinBox::editingFinished, this, [this]() { commitScale(2); });

    resetState();
}

void InspectorPanel::setContext(Scene::Document* document, GeometryKernel* kernel, Core::CommandStack* stack)
{
    currentDocument = document;
    currentKernel = kernel;
    commandStack = stack;
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

void InspectorPanel::setDocument(Scene::Document* document)
{
    currentDocument = document;
}

void InspectorPanel::setCommandStack(Core::CommandStack* stack)
{
    commandStack = stack;
}

void InspectorPanel::rebuildGeneralProperties()
{
    selectionIds.clear();
    selectionTransforms.clear();

    if (!generalSection) {
        updateTransformEditors();
        return;
    }

    if (!currentDocument || currentSelection.empty()) {
        generalSection->setVisible(false);
        if (transformSection)
            transformSection->setVisible(false);
        updateTransformEditors();
        return;
    }

    std::unordered_set<Scene::Document::ObjectId> seen;
    seen.reserve(currentSelection.size());
    selectionIds.reserve(currentSelection.size());
    selectionTransforms.reserve(currentSelection.size());

    for (GeometryObject* object : currentSelection) {
        if (!object)
            continue;
        Scene::Document::ObjectId id = currentDocument->objectIdForGeometry(object);
        if (id == 0)
            continue;
        if (!seen.insert(id).second)
            continue;
        selectionIds.push_back(id);
        selectionTransforms.push_back(currentDocument->objectTransform(id));
    }

    if (selectionIds.empty()) {
        generalSection->setVisible(false);
        if (transformSection)
            transformSection->setVisible(false);
        updateTransformEditors();
        return;
    }

    generalSection->setVisible(true);
    if (transformSection)
        transformSection->setVisible(true);

    // Name field
    nameMixed = false;
    currentNameValue.clear();
    bool nameInitialized = false;
    for (Scene::Document::ObjectId id : selectionIds) {
        const auto* node = currentDocument->findObject(id);
        if (!node)
            continue;
        QString candidate = QString::fromStdString(node->name);
        if (!nameInitialized) {
            currentNameValue = candidate;
            nameInitialized = true;
        } else if (candidate != currentNameValue) {
            nameMixed = true;
            break;
        }
    }
    if (nameEdit) {
        QSignalBlocker blocker(nameEdit);
        if (nameMixed)
            nameEdit->setText(mixedValueDisplay);
        else
            nameEdit->setText(currentNameValue);
        nameEdit->setEnabled(true);
    }

    // Tags display
    if (tagLabel) {
        bool tagsInitialized = false;
        bool tagsMixed = false;
        std::vector<Scene::Document::TagId> baseline;
        for (Scene::Document::ObjectId id : selectionIds) {
            const auto* node = currentDocument->findObject(id);
            if (!node)
                continue;
            if (!tagsInitialized) {
                baseline = node->tags;
                tagsInitialized = true;
            } else if (node->tags != baseline) {
                tagsMixed = true;
                break;
            }
        }
        QString text;
        if (tagsMixed) {
            text = mixedValueDisplay;
        } else if (tagsInitialized && !baseline.empty()) {
            QStringList names;
            for (Scene::Document::TagId tagId : baseline) {
                auto it = currentDocument->tags().find(tagId);
                if (it != currentDocument->tags().end())
                    names.append(QString::fromStdString(it->second.name));
            }
            text = names.isEmpty() ? tr("<None>") : names.join(QStringLiteral(", "));
        } else {
            text = tr("<None>");
        }
        tagLabel->setText(text);
    }

    // Visibility state
    visibilityMixed = false;
    visibilityValue = true;
    bool visibilityInitialized = false;
    for (Scene::Document::ObjectId id : selectionIds) {
        if (const auto* node = currentDocument->findObject(id)) {
            if (!visibilityInitialized) {
                visibilityValue = node->visible;
                visibilityInitialized = true;
            } else if (node->visible != visibilityValue) {
                visibilityMixed = true;
                break;
            }
        }
    }
    if (visibleCheck) {
        QSignalBlocker blocker(visibleCheck);
        visibleCheck->setEnabled(true);
        visibleCheck->setTristate(visibilityMixed);
        if (visibilityMixed)
            visibleCheck->setCheckState(Qt::PartiallyChecked);
        else
            visibleCheck->setCheckState(visibilityValue ? Qt::Checked : Qt::Unchecked);
    }

    // Lock state will be updated once document exposes it; default to unchecked/disabled for now.
    lockMixed = false;
    lockValue = false;
    if (lockCheck) {
        QSignalBlocker blocker(lockCheck);
        lockCheck->setEnabled(false);
        lockCheck->setTristate(false);
        lockCheck->setCheckState(Qt::Unchecked);
    }

    // Materials
    materialMixed = false;
    currentMaterialValue.clear();
    bool materialInitialized = false;
    for (GeometryObject* object : currentSelection) {
        if (!object)
            continue;
        QString value = QString::fromStdString(currentDocument->geometry().getMaterial(object));
        if (!materialInitialized) {
            currentMaterialValue = value;
            materialInitialized = true;
        } else if (value != currentMaterialValue) {
            materialMixed = true;
            break;
        }
    }
    refreshMaterialChoices();
    if (materialCombo) {
        QSignalBlocker blocker(materialCombo);
        if (materialMixed) {
            materialCombo->setEditText(mixedValueDisplay);
        } else if (!currentMaterialValue.isEmpty()) {
            int index = materialCombo->findData(currentMaterialValue, Qt::UserRole);
            if (index < 0)
                index = materialCombo->findText(currentMaterialValue, Qt::MatchExactly);
            if (index >= 0)
                materialCombo->setCurrentIndex(index);
            else
                materialCombo->setEditText(currentMaterialValue);
        } else {
            int noneIndex = materialCombo->findData(QString(), Qt::UserRole);
            if (noneIndex >= 0)
                materialCombo->setCurrentIndex(noneIndex);
            else
                materialCombo->setEditText(QString());
        }
    }

    // Transform fields
    if (selectionTransforms.empty())
        selectionTransforms.resize(selectionIds.size());
    referenceTransform = selectionTransforms.front();
    positionMixed = { false, false, false };
    for (std::size_t i = 1; i < selectionTransforms.size(); ++i) {
        const Vector3& point = selectionTransforms[i].position;
        if (!positionMixed[0] && std::fabs(point.x - referenceTransform.position.x) > kMetadataFloatEpsilon)
            positionMixed[0] = true;
        if (!positionMixed[1] && std::fabs(point.y - referenceTransform.position.y) > kMetadataFloatEpsilon)
            positionMixed[1] = true;
        if (!positionMixed[2] && std::fabs(point.z - referenceTransform.position.z) > kMetadataFloatEpsilon)
            positionMixed[2] = true;
    }
    updateTransformEditors();
}

void InspectorPanel::refreshMaterialChoices()
{
    if (!materialCombo)
        return;

    QSignalBlocker blocker(materialCombo);
    QString existing = materialCombo->currentText();
    materialCombo->clear();
    materialCombo->setEnabled(!selectionIds.empty());
    materialCombo->addItem(tr("<None>"), QString());

    std::unordered_set<QString> unique;
    if (currentDocument) {
        const auto& assignments = currentDocument->geometry().getMaterials();
        unique.reserve(assignments.size());
        for (const auto& entry : assignments) {
            if (entry.second.empty())
                continue;
            unique.insert(QString::fromStdString(entry.second));
        }
    }
    if (!currentMaterialValue.isEmpty())
        unique.insert(currentMaterialValue);

    std::vector<QString> sorted(unique.begin(), unique.end());
    std::sort(sorted.begin(), sorted.end(), [](const QString& a, const QString& b) {
        return a.localeAwareCompare(b) < 0;
    });
    for (const QString& value : sorted)
        materialCombo->addItem(value, value);

    materialCombo->setEditText(existing);
}

void InspectorPanel::updateTransformEditors()
{
    if (positionEditors.x) {
        bool enabled = !selectionIds.empty();
        positionEditors.x->setEnabled(enabled);
        positionEditors.y->setEnabled(enabled);
        positionEditors.z->setEnabled(enabled);
        setVectorEditors(positionEditors, referenceTransform.position, positionMixed);
    }
    std::array<bool, 3> alwaysMixed { true, true, true };
    setVectorEditors(rotationEditors, Vector3(), alwaysMixed);
    setVectorEditors(scaleEditors, Vector3(1.0f, 1.0f, 1.0f), alwaysMixed);
}

void InspectorPanel::applyPositionChange(int axis, QLineEdit* editor)
{
    if (updating || !editor)
        return;
    if (!commandStack || selectionIds.empty() || !currentDocument)
        return;

    QString text = editor->text().trimmed();
    if (text.isEmpty() || text == mixedValueDisplay) {
        updateTransformEditors();
        return;
    }

    Measurement::ParseResult parsed = Measurement::parseDistanceFlexible(text);
    if (!parsed.ok) {
        updateTransformEditors();
        return;
    }

    double base = 0.0;
    switch (axis) {
    case 0:
        base = referenceTransform.position.x;
        break;
    case 1:
        base = referenceTransform.position.y;
        break;
    default:
        base = referenceTransform.position.z;
        break;
    }

    double deltaComponent = parsed.value - base;
    if (std::fabs(deltaComponent) <= kMetadataFloatEpsilon) {
        updateTransformEditors();
        return;
    }

    std::vector<Scene::SetObjectTransformCommand::TransformChange> changes;
    changes.reserve(selectionIds.size());
    Scene::Document::TransformMask mask;
    if (axis >= 0 && axis < 3)
        mask.position[static_cast<std::size_t>(axis)] = true;

    for (Scene::Document::ObjectId id : selectionIds) {
        Scene::Document::Transform before = currentDocument->objectTransform(id);
        Scene::Document::Transform after = before;
        switch (axis) {
        case 0:
            after.position.x = before.position.x + static_cast<float>(deltaComponent);
            break;
        case 1:
            after.position.y = before.position.y + static_cast<float>(deltaComponent);
            break;
        default:
            after.position.z = before.position.z + static_cast<float>(deltaComponent);
            break;
        }
        changes.push_back({ id, before, after, mask });
    }

    if (changes.empty()) {
        updateTransformEditors();
        return;
    }

    auto command = std::make_unique<Scene::SetObjectTransformCommand>(std::move(changes), tr("Move Selection"));
    commandStack->push(std::move(command));

    updating = true;
    rebuildGeneralProperties();
    updating = false;
}

void InspectorPanel::setVectorEditors(VectorEditors& editors, const Vector3& value, const std::array<bool, 3>& mixedFlags)
{
    auto apply = [&](QLineEdit* edit, float component, bool mixed) {
        if (!edit)
            return;
        QSignalBlocker blocker(edit);
        if (mixed)
            edit->setText(mixedValueDisplay);
        else
            edit->setText(QString::number(component, 'f', 3));
    };
    apply(editors.x, value.x, mixedFlags[0]);
    apply(editors.y, value.y, mixedFlags[1]);
    apply(editors.z, value.z, mixedFlags[2]);
}

void InspectorPanel::commitName()
{
    if (updating || !nameEdit || !commandStack || selectionIds.empty())
        return;

    QString text = nameEdit->text().trimmed();
    if (text == mixedValueDisplay)
        return;

    if (!nameMixed && text == currentNameValue)
        return;

    if (selectionIds.size() == 1) {
        auto command = std::make_unique<Scene::RenameObjectCommand>(selectionIds.front(), text);
        commandStack->push(std::move(command));
    } else {
        auto command = std::make_unique<Scene::RenameObjectsCommand>(selectionIds, text);
        commandStack->push(std::move(command));
    }

    updating = true;
    rebuildGeneralProperties();
    updating = false;
}

void InspectorPanel::commitVisibility(int state)
{
    if (updating || !visibleCheck || !commandStack || selectionIds.empty())
        return;
    if (state == Qt::PartiallyChecked)
        return;

    bool desired = state == Qt::Checked;
    if (!visibilityMixed && desired == visibilityValue)
        return;

    if (selectionIds.size() == 1) {
        auto command = std::make_unique<Scene::SetObjectVisibilityCommand>(selectionIds.front(), desired);
        commandStack->push(std::move(command));
    } else {
        auto command = std::make_unique<Scene::SetObjectsVisibilityCommand>(selectionIds, desired);
        commandStack->push(std::move(command));
    }

    updating = true;
    rebuildGeneralProperties();
    updating = false;
}

void InspectorPanel::commitLock(int state)
{
    Q_UNUSED(state);
    // Lock editing is not yet supported in the document model.
    if (lockCheck)
        lockCheck->setCheckState(Qt::Unchecked);
}

void InspectorPanel::commitMaterial(const QString& text)
{
    if (updating || !materialCombo || !commandStack || selectionIds.empty())
        return;

    QString materialValue;
    bool fromItem = false;
    int index = materialCombo->currentIndex();
    if (index >= 0) {
        QVariant data = materialCombo->itemData(index, Qt::UserRole);
        if (data.isValid()) {
            materialValue = data.toString();
            fromItem = true;
        }
    }
    if (!fromItem)
        materialValue = text.trimmed();

    if (materialValue == mixedValueDisplay)
        return;

    if (!materialMixed && materialValue == currentMaterialValue)
        return;

    auto command = std::make_unique<Scene::AssignMaterialCommand>(selectionIds, materialValue);
    commandStack->push(std::move(command));

    updating = true;
    rebuildGeneralProperties();
    updating = false;
}

void InspectorPanel::commitPositionX()
{
    applyPositionChange(0, positionEditors.x);
}

void InspectorPanel::commitPositionY()
{
    applyPositionChange(1, positionEditors.y);
}

void InspectorPanel::commitPositionZ()
{
    applyPositionChange(2, positionEditors.z);
}

void InspectorPanel::resetState()
{
    updating = true;
    currentSelection.clear();
    currentSelectionIds.clear();
    currentObject = nullptr;
    currentMetadata = GeometryKernel::ShapeMetadata{};
    currentCenter = Vector3();
    nameMixed = false;
    visibilityMixed = false;
    materialMixed = false;
    if (nameEdit)
        nameEdit->clear();
    if (visibleCheck)
        visibleCheck->setCheckState(Qt::Unchecked);
    if (tagList)
        tagList->clear();
    if (materialCombo)
        materialCombo->clear();
    if (positionX) {
        positionX->setValue(0.0);
        positionY->setValue(0.0);
        positionZ->setValue(0.0);
    }
    if (rotationH) {
        rotationH->setValue(0.0);
        rotationP->setValue(0.0);
        rotationB->setValue(0.0);
    }
    if (scaleX) {
        scaleX->setValue(1.0);
        scaleY->setValue(1.0);
        scaleZ->setValue(1.0);
    }
    titleLabel->setText(tr("Inspector"));
    stacked->setCurrentWidget(nonePage);
    updating = false;
}

void InspectorPanel::updateSelection(Scene::Document* document,
                                     GeometryKernel* kernel,
                                     const std::vector<GeometryObject*>& selection)
{
    if (updating)
        return;

    currentDocument = document;
    currentKernel = kernel;

    if (!currentDocument || !currentKernel || selection.empty()) {
        resetState();
        if (noneLabel)
            noneLabel->setText(tr("Select a shape to view its properties."));
        return;
    }

    updating = true;
    currentSelection = selection;
    currentSelectionIds.clear();
    currentSelectionIds.reserve(selection.size());
    for (GeometryObject* object : selection) {
        Scene::Document::ObjectId id = currentDocument->objectIdForGeometry(object);
        if (id != 0)
            currentSelectionIds.push_back(id);
    }

    if (currentSelectionIds.empty()) {
        resetState();
        updating = false;
        return;
    }

    if (selection.size() == 1) {
        currentObject = selection.front();
        if (currentObject && currentKernel) {
            auto metadata = currentKernel->shapeMetadata(currentObject);
            if (metadata.has_value())
                currentMetadata = *metadata;
            else
                currentMetadata = GeometryKernel::ShapeMetadata{};
        }
    } else {
        currentObject = nullptr;
        currentMetadata = GeometryKernel::ShapeMetadata{};
    }

    currentCenter = combinedSelectionCenter();
    updating = false;

    populateGeneralInfo();
    populateFromMetadata();
}

void InspectorPanel::populateGeneralInfo()
{
    if (!stacked || !detailsPage) {
        return;
    }

    stacked->setCurrentWidget(detailsPage);

    if (!generalSection)
        return;

    updating = true;

    // Name
    QString nameValue;
    nameMixed = false;
    if (!currentSelectionIds.empty() && currentDocument) {
        const Scene::Document::ObjectNode* firstNode = currentDocument->findObject(currentSelectionIds.front());
        if (firstNode)
            nameValue = QString::fromStdString(firstNode->name);
        for (std::size_t i = 1; i < currentSelectionIds.size(); ++i) {
            const Scene::Document::ObjectNode* node = currentDocument->findObject(currentSelectionIds[i]);
            if (!node)
                continue;
            if (QString::fromStdString(node->name) != nameValue) {
                nameMixed = true;
                break;
            }
        }
    }
    if (nameEdit) {
        QSignalBlocker blocker(nameEdit);
        if (nameMixed)
            nameEdit->clear();
        else
            nameEdit->setText(nameValue);
        nameEdit->setEnabled(!currentSelectionIds.empty());
    }

    // Visibility
    visibilityMixed = false;
    bool anyVisible = false;
    bool allVisible = true;
    if (currentDocument) {
        for (Scene::Document::ObjectId id : currentSelectionIds) {
            const Scene::Document::ObjectNode* node = currentDocument->findObject(id);
            if (!node)
                continue;
            anyVisible = anyVisible || node->visible;
            allVisible = allVisible && node->visible;
        }
    }
    Qt::CheckState visibilityState = Qt::Unchecked;
    if (allVisible)
        visibilityState = Qt::Checked;
    else if (anyVisible)
        visibilityState = Qt::PartiallyChecked;
    visibilityMixed = (visibilityState == Qt::PartiallyChecked);
    if (visibleCheck) {
        QSignalBlocker blocker(visibleCheck);
        visibleCheck->setTristate(true);
        visibleCheck->setCheckState(visibilityState);
        visibleCheck->setEnabled(!currentSelectionIds.empty());
    }

    rebuildTagList();
    populateMaterials();
    populateTransforms();

    updating = false;
}

void InspectorPanel::populateFromMetadata()
{
    if (!typeStack)
        return;

    updating = true;
    switch (currentMetadata.type) {
    case GeometryKernel::ShapeMetadata::Type::Circle:
        titleLabel->setText(tr("Circle"));
        typeStack->setCurrentWidget(circlePage);
        populateCircle();
        break;
    case GeometryKernel::ShapeMetadata::Type::Polygon:
        titleLabel->setText(tr("Polygon"));
        typeStack->setCurrentWidget(polygonPage);
        populatePolygon();
        break;
    case GeometryKernel::ShapeMetadata::Type::Arc:
        titleLabel->setText(tr("Arc"));
        typeStack->setCurrentWidget(arcPage);
        populateArc();
        break;
    case GeometryKernel::ShapeMetadata::Type::Bezier:
        titleLabel->setText(tr("Bézier"));
        typeStack->setCurrentWidget(bezierPage);
        populateBezier();
        break;
    default:
        if (!currentSelection.empty())
            titleLabel->setText(tr("Selection"));
        else
            titleLabel->setText(tr("Inspector"));
        typeStack->setCurrentWidget(typeNonePage);
        if (typeNoneLabel) {
            if (selectionHasCurvesOnly())
                typeNoneLabel->setText(tr("The selected curves do not expose parametric metadata."));
            else
                typeNoneLabel->setText(tr("No additional parameters for the current selection."));
        }
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
    double sweep = toDegrees(data.endAngle - data.startAngle);
    if (!data.counterClockwise && sweep > 0.0)
        sweep = -sweep;
    if (data.counterClockwise && sweep < 0.0)
        sweep += 360.0;
    {
        QSignalBlocker blocker(arcSweep);
        arcSweep->setValue(std::max(kSweepMin, std::fabs(sweep)));
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

void InspectorPanel::rebuildTagList()
{
    if (!tagList) {
        return;
    }

    QSignalBlocker blocker(tagList);
    tagList->clear();

    if (!currentDocument) {
        tagList->setEnabled(false);
        return;
    }

    tagList->setEnabled(true);

    std::vector<std::pair<QString, Scene::Document::TagId>> tags;
    tags.reserve(currentDocument->tags().size());
    for (const auto& entry : currentDocument->tags()) {
        tags.emplace_back(QString::fromStdString(entry.second.name), entry.first);
    }
    std::sort(tags.begin(), tags.end(), [](const auto& a, const auto& b) { return a.first.localeAwareCompare(b.first) < 0; });

    if (tags.empty()) {
        auto* item = new QListWidgetItem(tr("No tags defined"), tagList);
        item->setFlags(Qt::NoItemFlags);
        return;
    }

    for (const auto& pair : tags) {
        Scene::Document::TagId tagId = pair.second;
        int assignedCount = 0;
        for (Scene::Document::ObjectId objectId : currentSelectionIds) {
            const Scene::Document::ObjectNode* node = currentDocument->findObject(objectId);
            if (!node)
                continue;
            if (std::find(node->tags.begin(), node->tags.end(), tagId) != node->tags.end())
                ++assignedCount;
        }
        Qt::CheckState state = Qt::Unchecked;
        if (assignedCount == static_cast<int>(currentSelectionIds.size()))
            state = Qt::Checked;
        else if (assignedCount > 0)
            state = Qt::PartiallyChecked;
        auto* item = new QListWidgetItem(pair.first, tagList);
        item->setFlags((item->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsSelectable);
        item->setData(Qt::UserRole, QVariant::fromValue(static_cast<qulonglong>(tagId)));
        item->setCheckState(state);
    }
}

void InspectorPanel::populateMaterials()
{
    if (!materialCombo) {
        return;
    }

    QSignalBlocker blocker(materialCombo);
    materialCombo->clear();

    std::set<QString> available;
    if (currentDocument) {
        const auto& assignments = currentDocument->geometry().getMaterials();
        for (const auto& entry : assignments) {
            if (!entry.second.empty())
                available.insert(QString::fromStdString(entry.second));
        }
    }

    QString selectedMaterial;
    materialMixed = false;
    bool first = true;
    if (currentKernel) {
        for (GeometryObject* object : currentSelection) {
            QString material = QString::fromStdString(currentKernel->getMaterial(object));
            if (!material.isEmpty())
                available.insert(material);
            if (first) {
                selectedMaterial = material;
                first = false;
            } else if (material != selectedMaterial) {
                materialMixed = true;
            }
        }
    }

    materialCombo->addItem(mixedPlaceholder(), QVariant());
    materialCombo->setItemData(0, 0, Qt::UserRole - 1);
    materialCombo->addItem(tr("None"), QString());
    for (const QString& material : available)
        materialCombo->addItem(material, material);

    if (materialMixed)
        materialCombo->setCurrentIndex(0);
    else {
        int index = selectedMaterial.isEmpty() ? materialCombo->findData(QString()) : materialCombo->findData(selectedMaterial);
        if (index >= 0)
            materialCombo->setCurrentIndex(index);
    }
    materialCombo->setEnabled(!currentSelectionIds.empty());
}

void InspectorPanel::populateTransforms()
{
    if (!positionX || !positionY || !positionZ || !rotationH || !rotationP || !rotationB || !scaleX || !scaleY || !scaleZ)
        return;

    BoundingBox box = combinedBoundingBox();
    bool hasValidBounds = box.valid;
    if (hasValidBounds)
        currentCenter = toVector3(box);
    else
        currentCenter = Vector3();

    {
        QSignalBlocker bx(positionX);
        QSignalBlocker by(positionY);
        QSignalBlocker bz(positionZ);
        positionX->setValue(currentCenter.x);
        positionY->setValue(currentCenter.y);
        positionZ->setValue(currentCenter.z);
    }
    positionX->setEnabled(hasValidBounds);
    positionY->setEnabled(hasValidBounds);
    positionZ->setEnabled(hasValidBounds);

    {
        QSignalBlocker bh(rotationH);
        QSignalBlocker bp(rotationP);
        QSignalBlocker bb(rotationB);
        rotationH->setValue(0.0);
        rotationP->setValue(0.0);
        rotationB->setValue(0.0);
    }
    rotationH->setEnabled(hasValidBounds);
    rotationP->setEnabled(hasValidBounds);
    rotationB->setEnabled(hasValidBounds);

    {
        QSignalBlocker sx(scaleX);
        QSignalBlocker sy(scaleY);
        QSignalBlocker sz(scaleZ);
        scaleX->setValue(1.0);
        scaleY->setValue(1.0);
        scaleZ->setValue(1.0);
    }
    scaleX->setEnabled(hasValidBounds);
    scaleY->setEnabled(hasValidBounds);
    scaleZ->setEnabled(hasValidBounds);
}

void InspectorPanel::commitName()
{
    if (updating || !nameEdit || currentSelectionIds.empty())
        return;

    QString newName = nameEdit->text().trimmed();
    if (newName.isEmpty())
        return;

    if (commandStack) {
        std::unique_ptr<Core::Command> command;
        if (currentSelectionIds.size() == 1)
            command = std::make_unique<Scene::RenameObjectCommand>(currentSelectionIds.front(), newName);
        else
            command = std::make_unique<Scene::RenameObjectsCommand>(currentSelectionIds, newName);
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentDocument)
        return;
    for (Scene::Document::ObjectId id : currentSelectionIds)
        currentDocument->renameObject(id, newName.toStdString());
    refreshAfterCommand();
}

void InspectorPanel::commitVisibility(Qt::CheckState state)
{
    if (updating || currentSelectionIds.empty())
        return;

    if (state == Qt::PartiallyChecked && visibilityMixed)
        return;

    bool visible = (state != Qt::Unchecked);

    if (commandStack) {
        std::unique_ptr<Core::Command> command;
        if (currentSelectionIds.size() == 1)
            command = std::make_unique<Scene::SetObjectVisibilityCommand>(currentSelectionIds.front(), visible);
        else
            command = std::make_unique<Scene::SetObjectsVisibilityCommand>(currentSelectionIds, visible);
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentDocument)
        return;
    for (Scene::Document::ObjectId id : currentSelectionIds)
        currentDocument->setObjectVisible(id, visible);
    refreshAfterCommand();
}

void InspectorPanel::commitMaterial(int index)
{
    if (updating || !materialCombo || currentSelectionIds.empty())
        return;

    if (index == 0 && materialMixed)
        return;

    QString materialName = materialCombo->itemData(index).toString();
    bool isPlaceholder = (index == 0 && materialCombo->itemData(index, Qt::UserRole - 1).isValid() &&
                          materialCombo->itemData(index, Qt::UserRole - 1).toInt() == 0);
    if (isPlaceholder)
        return;

    if (commandStack) {
        auto command = std::make_unique<Scene::AssignMaterialCommand>(currentSelectionIds, materialName);
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentDocument)
        return;
    for (Scene::Document::ObjectId id : currentSelectionIds) {
        GeometryObject* object = currentDocument->geometryForObject(id);
        if (!object)
            continue;
        currentDocument->geometry().assignMaterial(object, materialName.toStdString());
    }
    refreshAfterCommand();
}

void InspectorPanel::commitTagChange(QListWidgetItem* item)
{
    if (updating || !item || currentSelectionIds.empty())
        return;

    QVariant data = item->data(Qt::UserRole);
    if (!data.isValid())
        return;
    Scene::Document::TagId tagId = static_cast<Scene::Document::TagId>(data.toULongLong());
    bool assign = item->checkState() == Qt::Checked;

    if (commandStack) {
        auto command = std::make_unique<Scene::SetTagAssignmentsCommand>(tagId, currentSelectionIds, assign);
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentDocument)
        return;
    for (Scene::Document::ObjectId id : currentSelectionIds) {
        if (assign)
            currentDocument->assignTag(id, tagId);
        else
            currentDocument->removeTag(id, tagId);
    }
    refreshAfterCommand();
}

void InspectorPanel::commitPosition(int axis)
{
    if (updating || currentSelectionIds.empty())
        return;

    double target = 0.0;
    switch (axis) {
    case 0:
        target = positionX->value();
        break;
    case 1:
        target = positionY->value();
        break;
    case 2:
        target = positionZ->value();
        break;
    default:
        return;
    }

    double currentValue = (axis == 0 ? currentCenter.x : (axis == 1 ? currentCenter.y : currentCenter.z));
    double delta = target - currentValue;
    if (std::fabs(delta) < 1e-6)
        return;

    Vector3 translation(0.0f, 0.0f, 0.0f);
    if (axis == 0)
        translation.x = static_cast<float>(delta);
    else if (axis == 1)
        translation.y = static_cast<float>(delta);
    else
        translation.z = static_cast<float>(delta);

    applyTranslationDelta(translation, tr("Move Selection"));
}

void InspectorPanel::commitRotation(int axis)
{
    if (updating || currentSelectionIds.empty() || !rotationH || !rotationP || !rotationB)
        return;

    double value = 0.0;
    QDoubleSpinBox* spin = nullptr;
    switch (axis) {
    case 0:
        spin = rotationH;
        value = rotationH->value();
        break;
    case 1:
        spin = rotationP;
        value = rotationP->value();
        break;
    case 2:
        spin = rotationB;
        value = rotationB->value();
        break;
    default:
        return;
    }

    if (std::fabs(value) < 1e-6)
        return;

    applyRotationDelta(axis, value);
    {
        QSignalBlocker blocker(spin);
        spin->setValue(0.0);
    }
}

void InspectorPanel::commitScale(int axis)
{
    if (updating || currentSelectionIds.empty() || !scaleX || !scaleY || !scaleZ)
        return;

    double factor = 1.0;
    QDoubleSpinBox* spin = nullptr;
    switch (axis) {
    case 0:
        spin = scaleX;
        factor = scaleX->value();
        break;
    case 1:
        spin = scaleY;
        factor = scaleY->value();
        break;
    case 2:
        spin = scaleZ;
        factor = scaleZ->value();
        break;
    default:
        return;
    }

    if (std::fabs(factor - 1.0) < 1e-6)
        return;

    if (factor <= 0.0)
        factor = kScaleMin;

    applyScaleDelta(axis, factor);
    {
        QSignalBlocker blocker(spin);
        spin->setValue(1.0);
    }
}

void InspectorPanel::applyTranslationDelta(const Vector3& delta, const QString& description)
{
    if (delta.lengthSquared() <= 1e-10f)
        return;

    bool executed = false;
    if (commandStack) {
        auto command = std::make_unique<Tools::TranslateObjectsCommand>(currentSelectionIds, delta, description);
        commandStack->push(std::move(command));
        executed = true;
    } else if (currentDocument) {
        for (Scene::Document::ObjectId id : currentSelectionIds) {
            GeometryObject* object = currentDocument->geometryForObject(id);
            if (!object)
                continue;
            translateObject(*object, delta);
        }
        executed = true;
    }

    if (executed)
        refreshAfterCommand();
}

void InspectorPanel::applyRotationDelta(int axis, double degrees)
{
    if (std::fabs(degrees) < 1e-6 || !currentDocument)
        return;

    Vector3 axisVector(0.0f, 0.0f, 0.0f);
    switch (axis) {
    case 0: // Heading (yaw) around Y
        axisVector.y = 1.0f;
        break;
    case 1: // Pitch around X
        axisVector.x = 1.0f;
        break;
    case 2: // Bank around Z
        axisVector.z = 1.0f;
        break;
    default:
        return;
    }

    Vector3 pivot = currentCenter;
    float radians = static_cast<float>(qDegreesToRadians(degrees));

    bool executed = false;
    if (commandStack) {
        auto command = std::make_unique<Tools::RotateObjectsCommand>(currentSelectionIds, pivot, axisVector, radians, tr("Rotate Selection"));
        commandStack->push(std::move(command));
        executed = true;
    } else {
        for (Scene::Document::ObjectId id : currentSelectionIds) {
            GeometryObject* object = currentDocument->geometryForObject(id);
            if (!object)
                continue;
            rotateObject(*object, pivot, axisVector, radians);
        }
        executed = true;
    }

    if (executed)
        refreshAfterCommand();
}

void InspectorPanel::applyScaleDelta(int axis, double factor)
{
    if (!currentDocument)
        return;

    Vector3 factors(1.0f, 1.0f, 1.0f);
    switch (axis) {
    case 0:
        factors.x = static_cast<float>(factor);
        break;
    case 1:
        factors.y = static_cast<float>(factor);
        break;
    case 2:
        factors.z = static_cast<float>(factor);
        break;
    default:
        return;
    }

    Vector3 pivot = currentCenter;

    bool executed = false;
    if (commandStack) {
        auto command = std::make_unique<Tools::ScaleObjectsCommand>(currentSelectionIds, pivot, factors, tr("Scale Selection"));
        commandStack->push(std::move(command));
        executed = true;
    } else {
        for (Scene::Document::ObjectId id : currentSelectionIds) {
            GeometryObject* object = currentDocument->geometryForObject(id);
            if (!object)
                continue;
            scaleObject(*object, pivot, factors);
        }
        executed = true;
    }

    if (executed)
        refreshAfterCommand();
}

void InspectorPanel::refreshAfterCommand()
{
    if (updating)
        return;

    if (currentKernel && currentObject) {
        if (auto metadata = currentKernel->shapeMetadata(currentObject))
            currentMetadata = *metadata;
    }

    populateGeneralInfo();
    populateFromMetadata();
    emit shapeModified();
}

GeometryKernel::ShapeMetadata::Type InspectorPanel::currentMetadataType() const
{
    return currentMetadata.type;
}

bool InspectorPanel::selectionHasCurvesOnly() const
{
    if (!currentDocument)
        return false;
    for (GeometryObject* object : currentSelection) {
        if (!object || object->getType() != ObjectType::Curve)
            return false;
    }
    return !currentSelection.empty();
}

Vector3 InspectorPanel::combinedSelectionCenter() const
{
    BoundingBox box = combinedBoundingBox();
    if (!box.valid)
        return Vector3();
    return toVector3(box);
}

BoundingBox InspectorPanel::combinedBoundingBox() const
{
    BoundingBox combined;
    for (GeometryObject* object : currentSelection) {
        if (!object)
            continue;
        BoundingBox box = computeBoundingBox(*object);
        if (!box.valid)
            continue;
        if (!combined.valid) {
            combined = box;
            continue;
        }
        combined.min.x = std::min(combined.min.x, box.min.x);
        combined.min.y = std::min(combined.min.y, box.min.y);
        combined.min.z = std::min(combined.min.z, box.min.z);
        combined.max.x = std::max(combined.max.x, box.max.x);
        combined.max.y = std::max(combined.max.y, box.max.y);
        combined.max.z = std::max(combined.max.z, box.max.z);
    }
    return combined;
}

void InspectorPanel::commitCircle()
{
    if (updating || !currentKernel || !currentObject || currentSelectionIds.size() != 1)
        return;

    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    metadata.circle.radius = static_cast<float>(circleRadius->value());
    metadata.circle.segments = circleSegments->value();

    if (commandStack && currentDocument) {
        auto command = std::make_unique<Scene::RebuildCurveFromMetadataCommand>(currentSelectionIds.front(), metadata, tr("Update Circle"));
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populateCircle();
        return;
    metadata.circle.radius = radius;
    metadata.circle.segments = segments;
    bool applied = applyMetadata(metadata);
    populateCircle();
    if (applied)
        emit shapeModified();
}

void InspectorPanel::commitPolygon()
{
    if (updating || !currentKernel || !currentObject || currentSelectionIds.size() != 1)
        return;

    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    metadata.polygon.radius = static_cast<float>(polygonRadius->value());
    metadata.polygon.sides = polygonSides->value();

    if (commandStack && currentDocument) {
        auto command = std::make_unique<Scene::RebuildCurveFromMetadataCommand>(currentSelectionIds.front(), metadata, tr("Update Polygon"));
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populatePolygon();
        return;
    metadata.polygon.radius = radius;
    metadata.polygon.sides = sides;
    bool applied = applyMetadata(metadata);
    populatePolygon();
    if (applied)
        emit shapeModified();
}

void InspectorPanel::commitArc()
{
    if (updating || !currentKernel || !currentObject || currentSelectionIds.size() != 1)
        return;

    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    auto& def = metadata.arc.definition;
    float radius = static_cast<float>(arcRadius->value());
    float startAngle = qDegreesToRadians(static_cast<float>(arcStart->value()));
    double sweepDeg = arcSweep->value();
    double sweepRad = qDegreesToRadians(sweepDeg);
    bool ccw = arcDirection->isChecked();
    def.counterClockwise = ccw;
    def.endAngle = static_cast<float>(ccw ? def.startAngle + sweepRad : def.startAngle - sweepRad);
    def.segments = arcSegments->value();

    if (commandStack && currentDocument) {
        auto command = std::make_unique<Scene::RebuildCurveFromMetadataCommand>(currentSelectionIds.front(), metadata, tr("Update Arc"));
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populateArc();
        return;

    def.radius = radius;
    def.startAngle = startAngle;
    def.endAngle = endAngle;
    def.counterClockwise = ccw;
    def.segments = segments;
    bool applied = applyMetadata(metadata);
    populateArc();
    if (applied)
        emit shapeModified();
}

void InspectorPanel::commitBezier()
{
    if (updating || !currentKernel || !currentObject || currentSelectionIds.size() != 1)
        return;

    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    metadata.bezier.definition.p0 = pointFromEditors(bezierP0);
    metadata.bezier.definition.h0 = pointFromEditors(bezierH0);
    metadata.bezier.definition.h1 = pointFromEditors(bezierH1);
    metadata.bezier.definition.p1 = pointFromEditors(bezierP1);
    metadata.bezier.definition.segments = bezierSegments->value();

    if (commandStack && currentDocument) {
        auto command = std::make_unique<Scene::RebuildCurveFromMetadataCommand>(currentSelectionIds.front(), metadata, tr("Update Bézier"));
        commandStack->push(std::move(command));
        refreshAfterCommand();
        return;
    }

    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata)) {
        populateBezier();
        return;
    metadata.bezier.definition.p0 = p0;
    metadata.bezier.definition.h0 = h0;
    metadata.bezier.definition.h1 = h1;
    metadata.bezier.definition.p1 = p1;
    metadata.bezier.definition.segments = segments;
    bool applied = applyMetadata(metadata);
    populateBezier();
    if (applied)
        emit shapeModified();
}

