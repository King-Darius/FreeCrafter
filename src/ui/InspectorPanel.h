#pragma once

#include <QWidget>

#include <array>
#include <vector>

#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Vector3.h"
#include "../Tools/ToolGeometryUtils.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QLineEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
class QStackedWidget;

class GeometryKernel;
class GeometryObject;
class Vector3;

namespace Core {
class CommandStack;
}

namespace Scene {
class Document;
}

class InspectorPanel : public QWidget {
    Q_OBJECT
public:
    explicit InspectorPanel(QWidget* parent = nullptr);

    void setContext(Scene::Document* document, GeometryKernel* kernel, Core::CommandStack* stack);
    void updateSelection(Scene::Document* document,
                        GeometryKernel* kernel,
                        const std::vector<GeometryObject*>& selection);

signals:
    void shapeModified();

private slots:
    void commitName();
    void commitVisibility(int state);
    void commitLock(int state);
    void commitMaterial(const QString& text);
    void commitPositionX();
    void commitPositionY();
    void commitPositionZ();
    void commitCircle();
    void commitPolygon();
    void commitArc();
    void commitBezier();

private:
    struct PointEditors {
        QDoubleSpinBox* x = nullptr;
        QDoubleSpinBox* y = nullptr;
        QDoubleSpinBox* z = nullptr;
    };

    struct VectorEditors {
        QLineEdit* x = nullptr;
        QLineEdit* y = nullptr;
        QLineEdit* z = nullptr;
    };

    void rebuildGeneralProperties();
    void refreshMaterialChoices();
    void updateTransformEditors();
    void applyPositionChange(int axis, QLineEdit* editor);
    void setVectorEditors(VectorEditors& editors, const Vector3& value, const std::array<bool, 3>& mixedFlags);

    QWidget* createMessagePage(const QString& text, QLabel** labelOut);
    void populateGeneralInfo();
    void populateFromMetadata();
    void populateCircle();
    void populatePolygon();
    void populateArc();
    void populateBezier();
    void setPointEditors(const GeometryKernel::ShapeMetadata& metadata);
    void setPointEditors(PointEditors& editors, const Vector3& value);
    void connectPointEditors(PointEditors& editors);
    Vector3 pointFromEditors(const PointEditors& editors) const;
    void rebuildTagList();
    void populateMaterials();
    void populateTransforms();
    void commitName();
    void commitVisibility(Qt::CheckState state);
    void commitMaterial(int index);
    void commitTagChange(QListWidgetItem* item);
    void commitPosition(int axis);
    void commitRotation(int axis);
    void commitScale(int axis);
    void applyTranslationDelta(const Vector3& delta, const QString& description);
    void applyRotationDelta(int axis, double degrees);
    void applyScaleDelta(int axis, double factor);
    void refreshAfterCommand();
    GeometryKernel::ShapeMetadata::Type currentMetadataType() const;
    bool selectionHasCurvesOnly() const;
    void updateTypeStack();
    Vector3 combinedSelectionCenter() const;
    BoundingBox combinedBoundingBox() const;
    void resetState();
    bool applyMetadata(const GeometryKernel::ShapeMetadata& metadata);
    static bool fuzzyEqual(float a, float b);
    static bool vectorEqual(const Vector3& a, const Vector3& b);

    Scene::Document* currentDocument = nullptr;
    GeometryKernel* currentKernel = nullptr;
    std::vector<GeometryObject*> currentSelection;
    GeometryObject* currentObject = nullptr;
    Core::CommandStack* commandStack = nullptr;
    std::vector<GeometryObject*> currentSelection;
    std::vector<Scene::Document::ObjectId> currentSelectionIds;
    GeometryKernel::ShapeMetadata currentMetadata{};
    Vector3 currentCenter{ 0.0f, 0.0f, 0.0f };
    bool updating = false;
    bool nameMixed = false;
    bool visibilityMixed = false;
    bool materialMixed = false;

    QString mixedValueDisplay;
    QString currentNameValue;
    QString currentMaterialValue;
    bool nameMixed = false;
    bool visibilityMixed = false;
    bool lockMixed = false;
    bool materialMixed = false;
    bool visibilityValue = true;
    bool lockValue = false;
    std::array<bool, 3> positionMixed { false, false, false };
    Scene::Document::Transform referenceTransform{};
    std::vector<Scene::Document::Transform> selectionTransforms;
    std::vector<Scene::Document::ObjectId> selectionIds;

    QLabel* titleLabel = nullptr;
    QWidget* generalSection = nullptr;
    QFormLayout* generalForm = nullptr;
    QLineEdit* nameEdit = nullptr;
    QLabel* tagLabel = nullptr;
    QCheckBox* visibleCheck = nullptr;
    QCheckBox* lockCheck = nullptr;
    QComboBox* materialCombo = nullptr;
    QWidget* transformSection = nullptr;
    VectorEditors positionEditors;
    VectorEditors rotationEditors;
    VectorEditors scaleEditors;
    QStackedWidget* stacked = nullptr;

    QWidget* nonePage = nullptr;
    QLabel* noneLabel = nullptr;
    QWidget* detailsPage = nullptr;

    QWidget* generalSection = nullptr;
    QLineEdit* nameEdit = nullptr;
    QCheckBox* visibleCheck = nullptr;
    QListWidget* tagList = nullptr;
    QComboBox* materialCombo = nullptr;
    QDoubleSpinBox* positionX = nullptr;
    QDoubleSpinBox* positionY = nullptr;
    QDoubleSpinBox* positionZ = nullptr;
    QDoubleSpinBox* rotationH = nullptr;
    QDoubleSpinBox* rotationP = nullptr;
    QDoubleSpinBox* rotationB = nullptr;
    QDoubleSpinBox* scaleX = nullptr;
    QDoubleSpinBox* scaleY = nullptr;
    QDoubleSpinBox* scaleZ = nullptr;

    QStackedWidget* typeStack = nullptr;
    QWidget* typeNonePage = nullptr;
    QLabel* typeNoneLabel = nullptr;

    QWidget* circlePage = nullptr;
    QDoubleSpinBox* circleRadius = nullptr;
    QSpinBox* circleSegments = nullptr;

    QWidget* polygonPage = nullptr;
    QDoubleSpinBox* polygonRadius = nullptr;
    QSpinBox* polygonSides = nullptr;

    QWidget* arcPage = nullptr;
    QDoubleSpinBox* arcRadius = nullptr;
    QDoubleSpinBox* arcStart = nullptr;
    QDoubleSpinBox* arcSweep = nullptr;
    QSpinBox* arcSegments = nullptr;
    QCheckBox* arcDirection = nullptr;

    QWidget* bezierPage = nullptr;
    PointEditors bezierP0;
    PointEditors bezierH0;
    PointEditors bezierH1;
    PointEditors bezierP1;
    QSpinBox* bezierSegments = nullptr;
};

