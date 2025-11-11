#include "InspectorPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QDoubleValidator>
#include <QVariant>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <memory>

#include "../GeometryKernel/GeometryObject.h"
#include "Scene/SceneGraphCommands.h"
#include "../Scene/Document.h"
#include "../Core/CommandStack.h"
#include "../Tools/ToolCommands.h"

#include <unordered_set>

namespace {
constexpr double kDefaultMinRadius = 0.01;
constexpr double kMaxRadius = 100000.0;
constexpr double kAngleMin = 0.0;
constexpr double kAngleMax = 360.0;
constexpr double kSweepMin = 0.1;
constexpr double kSweepMax = 360.0;
// Inspector metadata editors work in float precision with up to three visible
// decimals, so treat values within one micro unit as equal. This keeps
// user-entered tweaks (even on large coordinates) while avoiding redundant
// updates from round-tripping through the spin boxes.
constexpr float kMetadataFloatEpsilon = 1e-6f;

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

    layout->addWidget(generalSection);

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
            auto* validator = new QDoubleValidator(-kMaxRadius, kMaxRadius, 3, edit);
            validator->setNotation(QDoubleValidator::StandardNotation);
            edit->setValidator(validator);
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

    layout->addWidget(transformSection);

    stacked = new QStackedWidget(this);
    layout->addWidget(stacked, 1);

    nonePage = createMessagePage(tr("Select a shape to view its properties."), &noneLabel);
    multiPage = createMessagePage(tr("Multiple selection detected. Curve parameters require a single selection."), &multiLabel);
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
    circleRadius->setObjectName(QStringLiteral("inspectorCircleRadius"));
    circleLayout->addRow(tr("Radius"), circleRadius);
    circleSegments = new QSpinBox(circlePage);
    circleSegments->setRange(8, 512);
    circleSegments->setSingleStep(1);
    circleSegments->setObjectName(QStringLiteral("inspectorCircleSegments"));
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
    connect(arcDirection, &QCheckBox::stateChanged, this, &InspectorPanel::commitArc);

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
    selectionCenters.clear();

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
    selectionCenters.reserve(currentSelection.size());

    for (GeometryObject* object : currentSelection) {
        if (!object)
            continue;
        Scene::Document::ObjectId id = currentDocument->objectIdForGeometry(object);
        if (id == 0)
            continue;
        if (!seen.insert(id).second)
            continue;
        selectionIds.push_back(id);
        if (auto centroid = centroidForObject(object))
            selectionCenters.push_back(*centroid);
        else
            selectionCenters.push_back(Vector3());
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
    if (selectionCenters.empty())
        selectionCenters.resize(selectionIds.size(), Vector3());
    referencePosition = selectionCenters.front();
    positionMixed = { false, false, false };
    for (std::size_t i = 1; i < selectionCenters.size(); ++i) {
        const Vector3& point = selectionCenters[i];
        if (!positionMixed[0] && std::fabs(point.x - referencePosition.x) > kMetadataFloatEpsilon)
            positionMixed[0] = true;
        if (!positionMixed[1] && std::fabs(point.y - referencePosition.y) > kMetadataFloatEpsilon)
            positionMixed[1] = true;
        if (!positionMixed[2] && std::fabs(point.z - referencePosition.z) > kMetadataFloatEpsilon)
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
        setVectorEditors(positionEditors, referencePosition, positionMixed);
    }
    std::array<bool, 3> alwaysMixed { true, true, true };
    setVectorEditors(rotationEditors, Vector3(), alwaysMixed);
    setVectorEditors(scaleEditors, Vector3(1.0f, 1.0f, 1.0f), alwaysMixed);
}

void InspectorPanel::applyPositionDelta(int axis, QLineEdit* editor)
{
    if (updating || !editor)
        return;
    if (!commandStack || selectionIds.empty())
        return;

    QString text = editor->text().trimmed();
    if (text.isEmpty() || text == mixedValueDisplay)
        return;

    bool ok = false;
    double value = text.toDouble(&ok);
    if (!ok) {
        updateTransformEditors();
        return;
    }

    double base = 0.0;
    switch (axis) {
    case 0:
        base = referencePosition.x;
        break;
    case 1:
        base = referencePosition.y;
        break;
    default:
        base = referencePosition.z;
        break;
    }

    double deltaComponent = value - base;
    if (std::fabs(deltaComponent) <= kMetadataFloatEpsilon)
        return;

    Vector3 delta(0.0f, 0.0f, 0.0f);
    if (axis == 0)
        delta.x = static_cast<float>(deltaComponent);
    else if (axis == 1)
        delta.y = static_cast<float>(deltaComponent);
    else
        delta.z = static_cast<float>(deltaComponent);

    auto ids = selectionIds;
    auto command = std::make_unique<Tools::TranslateObjectsCommand>(ids, delta, tr("Move Selection"));
    commandStack->push(std::move(command));

    updating = true;
    rebuildGeneralProperties();
    updating = false;
}

std::optional<Vector3> InspectorPanel::centroidForObject(const GeometryObject* object) const
{
    if (!object)
        return std::nullopt;
    const auto& vertices = object->getMesh().getVertices();
    if (vertices.empty())
        return std::nullopt;
    Vector3 sum(0.0f, 0.0f, 0.0f);
    for (const auto& vertex : vertices)
        sum += vertex.position;
    float inv = 1.0f / static_cast<float>(vertices.size());
    return sum * inv;
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
    applyPositionDelta(0, positionEditors.x);
}

void InspectorPanel::commitPositionY()
{
    applyPositionDelta(1, positionEditors.y);
}

void InspectorPanel::commitPositionZ()
{
    applyPositionDelta(2, positionEditors.z);
}

void InspectorPanel::resetState()
{
    updating = true;
    currentKernel = nullptr;
    currentSelection.clear();
    currentObject = nullptr;
    currentMetadata = GeometryKernel::ShapeMetadata{};
    selectionIds.clear();
    selectionCenters.clear();
    referencePosition = Vector3();
    nameMixed = false;
    visibilityMixed = false;
    lockMixed = false;
    materialMixed = false;
    positionMixed = { true, true, true };
    currentNameValue.clear();
    currentMaterialValue.clear();
    if (generalSection)
        generalSection->setVisible(false);
    if (transformSection)
        transformSection->setVisible(false);
    if (nameEdit) {
        QSignalBlocker blocker(nameEdit);
        nameEdit->clear();
    }
    if (tagLabel)
        tagLabel->setText(tr("<None>"));
    if (visibleCheck) {
        QSignalBlocker blocker(visibleCheck);
        visibleCheck->setTristate(false);
        visibleCheck->setCheckState(Qt::Unchecked);
    }
    if (lockCheck) {
        QSignalBlocker blocker(lockCheck);
        lockCheck->setTristate(false);
        lockCheck->setCheckState(Qt::Unchecked);
    }
    if (materialCombo) {
        QSignalBlocker blocker(materialCombo);
        materialCombo->clear();
        materialCombo->setEditText(QString());
    }
    updateTransformEditors();
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

    updating = true;
    currentKernel = kernel;
    currentSelection = selection;
    currentObject = nullptr;
    if (selection.size() == 1)
        currentObject = selection.front();

    rebuildGeneralProperties();

    if (selectionIds.empty()) {
        titleLabel->setText(tr("Inspector"));
        stacked->setCurrentWidget(nonePage);
        updating = false;
        return;
    }

    if (selection.size() > 1) {
        titleLabel->setText(tr("Selection (%1)").arg(selectionIds.size()));
        if (multiLabel)
            multiLabel->setText(tr("Multiple selection detected. Curve parameters require a single selection."));
        stacked->setCurrentWidget(multiPage);
        updating = false;
        return;
    }

    GeometryObject* candidate = currentObject;
    if (!candidate) {
        titleLabel->setText(tr("Inspector"));
        stacked->setCurrentWidget(unsupportedPage);
        if (unsupportedLabel)
            unsupportedLabel->setText(tr("The selected entity cannot be edited."));
        updating = false;
        return;
    }

    if (const Scene::Document::ObjectNode* node = currentDocument ? currentDocument->findObject(selectionIds.front()) : nullptr) {
        if (!currentNameValue.isEmpty() && !nameMixed)
            titleLabel->setText(currentNameValue);
        else if (!node->name.empty())
            titleLabel->setText(QString::fromStdString(node->name));
        else
            titleLabel->setText(tr("Selection"));
    }

    if (candidate->getType() != ObjectType::Curve) {
        currentMetadata = GeometryKernel::ShapeMetadata{};
        if (unsupportedLabel)
            unsupportedLabel->setText(tr("The selected entity cannot be edited."));
        stacked->setCurrentWidget(unsupportedPage);
        updating = false;
        return;
    }

    auto metadata = kernel->shapeMetadata(candidate);
    if (!metadata.has_value()) {
        currentMetadata = GeometryKernel::ShapeMetadata{};
        if (unsupportedLabel)
            unsupportedLabel->setText(tr("This curve was not created by a parametric tool."));
        stacked->setCurrentWidget(unsupportedPage);
        updating = false;
        return;
    }

    currentMetadata = *metadata;
    populateFromMetadata();
    updating = false;
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

bool InspectorPanel::fuzzyEqual(float a, float b)
{
    return std::fabs(static_cast<double>(a) - static_cast<double>(b)) <= kMetadataFloatEpsilon;
}

bool InspectorPanel::vectorEqual(const Vector3& a, const Vector3& b)
{
    return fuzzyEqual(a.x, b.x) && fuzzyEqual(a.y, b.y) && fuzzyEqual(a.z, b.z);
}

bool InspectorPanel::applyMetadata(const GeometryKernel::ShapeMetadata& metadata)
{
    if (!currentKernel || !currentObject)
        return false;

    if (commandStack && currentDocument) {
        Scene::Document::ObjectId objectId = currentDocument->objectIdForGeometry(currentObject);
        if (objectId == 0)
            return false;

        auto command = std::make_unique<Scene::RebuildCurveFromMetadataCommand>(objectId, metadata);
        auto* commandPtr = command.get();
        commandStack->push(std::move(command));
        if (!commandPtr->wasApplied())
            return false;

        if (auto refreshed = currentKernel->shapeMetadata(currentObject))
            currentMetadata = *refreshed;
        else
            currentMetadata = metadata;
        return true;
    }

    if (!currentKernel->rebuildShapeFromMetadata(currentObject, metadata))
        return false;

    if (auto refreshed = currentKernel->shapeMetadata(currentObject))
        currentMetadata = *refreshed;
    else
        currentMetadata = metadata;
    return true;
}

void InspectorPanel::commitCircle()
{
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    float radius = static_cast<float>(circleRadius->value());
    int segments = circleSegments->value();
    if (fuzzyEqual(radius, currentMetadata.circle.radius) && segments == currentMetadata.circle.segments)
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
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    float radius = static_cast<float>(polygonRadius->value());
    int sides = polygonSides->value();
    if (fuzzyEqual(radius, currentMetadata.polygon.radius) && sides == currentMetadata.polygon.sides)
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
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    auto& def = metadata.arc.definition;
    float radius = static_cast<float>(arcRadius->value());
    float startAngle = qDegreesToRadians(static_cast<float>(arcStart->value()));
    double sweepDeg = arcSweep->value();
    double sweepRad = qDegreesToRadians(sweepDeg);
    bool ccw = arcDirection->isChecked();
    float endAngle = ccw ? static_cast<float>(startAngle + sweepRad) : static_cast<float>(startAngle - sweepRad);
    int segments = arcSegments->value();

    const auto& currentDef = currentMetadata.arc.definition;
    if (fuzzyEqual(radius, currentDef.radius) && fuzzyEqual(startAngle, currentDef.startAngle) &&
        fuzzyEqual(endAngle, currentDef.endAngle) && segments == currentDef.segments &&
        ccw == currentDef.counterClockwise)
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
    if (updating || !currentKernel || !currentObject)
        return;
    GeometryKernel::ShapeMetadata metadata = currentMetadata;
    Vector3 p0 = pointFromEditors(bezierP0);
    Vector3 h0 = pointFromEditors(bezierH0);
    Vector3 h1 = pointFromEditors(bezierH1);
    Vector3 p1 = pointFromEditors(bezierP1);
    int segments = bezierSegments->value();
    const auto& currentDef = currentMetadata.bezier.definition;
    if (vectorEqual(p0, currentDef.p0) && vectorEqual(h0, currentDef.h0) && vectorEqual(h1, currentDef.h1) &&
        vectorEqual(p1, currentDef.p1) && segments == currentDef.segments)
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

