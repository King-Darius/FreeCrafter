#pragma once

#include <QWidget>

#include <vector>

#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Vector3.h"
#include "../Tools/ToolGeometryUtils.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
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

    Scene::Document* currentDocument = nullptr;
    GeometryKernel* currentKernel = nullptr;
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

    QLabel* titleLabel = nullptr;
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

