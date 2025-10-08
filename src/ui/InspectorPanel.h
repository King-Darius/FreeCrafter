#pragma once

#include <QWidget>

#include <vector>

#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Vector3.h"

class QCheckBox;
class QDoubleSpinBox;
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

    void updateSelection(GeometryKernel* kernel, const std::vector<GeometryObject*>& selection);

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

    GeometryKernel* currentKernel = nullptr;
    GeometryObject* currentObject = nullptr;
    GeometryKernel::ShapeMetadata currentMetadata{};
    bool updating = false;

    QLabel* titleLabel = nullptr;
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

