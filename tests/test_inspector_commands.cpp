#include <cassert>
#include <cmath>
#include <vector>

#include <QCoreApplication>
#include <QString>
#include <QUndoStack>

#include "Scene/Document.h"
#include "Scene/SceneGraphCommands.h"
#include "Core/CommandStack.h"
#include "GeometryKernel/Vector3.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    Scene::Document document;
    GeometryKernel& geometry = document.geometry();

    std::vector<Vector3> points { Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) };
    GeometryObject* curve = geometry.addCurve(points);
    assert(curve != nullptr);

    Scene::Document::ObjectId objectId = document.ensureObjectForGeometry(curve, "Edge");
    assert(objectId != 0);

    QUndoStack undoStack;
    Core::CommandStack commandStack(&undoStack);
    Core::CommandContext context;
    context.document = &document;
    context.geometry = &geometry;
    commandStack.setContext(context);

    GeometryObject* circle = createCircle(document, 1.0f, 32);
    assert(circle);

    InspectorPanel panel;
    panel.setDocument(&document);
    panel.setCommandStack(&commandStack);

    std::vector<GeometryObject*> selection{ circle };
    panel.updateSelection(&document.geometry(), selection);

    auto* radiusSpin = panel.findChild<QDoubleSpinBox*>(QStringLiteral("inspectorCircleRadius"));
    auto* segmentsSpin = panel.findChild<QSpinBox*>(QStringLiteral("inspectorCircleSegments"));
    assert(radiusSpin);
    assert(segmentsSpin);

    double originalRadius = radiusSpin->value();
    int originalSegments = segmentsSpin->value();

    double targetRadius = 2.5;
    int targetSegments = originalSegments + 12;

    radiusSpin->setValue(targetRadius);
    segmentsSpin->setValue(targetSegments);
    QMetaObject::invokeMethod(&panel, "commitCircle", Qt::DirectConnection);

    assert(undoStack.canUndo());
    assert(undoStack.undoText() == QStringLiteral("Edit Curve"));
    auto metadata = document.geometry().shapeMetadata(circle);
    assert(metadata.has_value());
    assert(std::fabs(metadata->circle.radius - static_cast<float>(targetRadius)) < kTolerance);
    assert(metadata->circle.segments == targetSegments);

    panel.updateSelection(&document.geometry(), selection);
    assert(std::fabs(radiusSpin->value() - targetRadius) < kTolerance);
    assert(segmentsSpin->value() == targetSegments);

    undoStack.undo();

    metadata = document.geometry().shapeMetadata(circle);
    assert(metadata.has_value());
    assert(std::fabs(metadata->circle.radius - static_cast<float>(originalRadius)) < kTolerance);
    assert(metadata->circle.segments == originalSegments);
    assert(undoStack.canRedo());

    panel.updateSelection(&document.geometry(), selection);
    assert(std::fabs(radiusSpin->value() - originalRadius) < kTolerance);
    assert(segmentsSpin->value() == originalSegments);

    undoStack.redo();

    metadata = document.geometry().shapeMetadata(circle);
    assert(metadata.has_value());
    assert(std::fabs(metadata->circle.radius - static_cast<float>(targetRadius)) < kTolerance);
    assert(metadata->circle.segments == targetSegments);

    panel.updateSelection(&document.geometry(), selection);
    assert(std::fabs(radiusSpin->value() - targetRadius) < kTolerance);
    assert(segmentsSpin->value() == targetSegments);

    return 0;
}

