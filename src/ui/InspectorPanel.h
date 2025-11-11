#pragma once

#include <QWidget>

#include <array>
#include <optional>
#include <vector>

#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Vector3.h"

namespace Core {
class CommandStack;
}

namespace Scene {
class Document;
}

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QLineEdit;
class QLabel;
class QSpinBox;
class QStackedWidget;

class GeometryKernel;
class GeometryObject;
class Vector3;

class InspectorPanel : public QWidget {
    Q_OBJECT
public:
    explicit InspectorPanel(QWidget* parent = nullptr);

    void setDocument(Scene::Document* document);
    void setCommandStack(Core::CommandStack* stack);
    void updateSelection(GeometryKernel* kernel, const std::vector<GeometryObject*>& selection);

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
    void applyPositionDelta(int axis, QLineEdit* editor);
    std::optional<Vector3> centroidForObject(const GeometryObject* object) const;
    void setVectorEditors(VectorEditors& editors, const Vector3& value, const std::array<bool, 3>& mixedFlags);

    QWidget* createMessagePage(const QString& text, QLabel** labelOut);
    void populateFromMetadata();
    void populateCircle();
    void populatePolygon();
    void populateArc();
    void populateBezier();
    void setPointEditors(const GeometryKernel::ShapeMetadata& metadata);
    void setPointEditors(PointEditors& editors, const Vector3& value);
    void connectPointEditors(PointEditors& editors);
    Vector3 pointFromEditors(const PointEditors& editors) const;
    void resetState();
    bool applyMetadata(const GeometryKernel::ShapeMetadata& metadata);
    static bool fuzzyEqual(float a, float b);
    static bool vectorEqual(const Vector3& a, const Vector3& b);

    GeometryKernel* currentKernel = nullptr;
    std::vector<GeometryObject*> currentSelection;
    GeometryObject* currentObject = nullptr;
    GeometryKernel::ShapeMetadata currentMetadata{};
    Scene::Document* currentDocument = nullptr;
    Core::CommandStack* commandStack = nullptr;
    bool updating = false;

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
    Vector3 referencePosition{};
    std::vector<Vector3> selectionCenters;
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
    QWidget* multiPage = nullptr;
    QWidget* unsupportedPage = nullptr;
    QLabel* noneLabel = nullptr;
    QLabel* multiLabel = nullptr;
    QLabel* unsupportedLabel = nullptr;

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

