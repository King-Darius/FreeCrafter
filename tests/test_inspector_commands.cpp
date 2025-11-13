#include <QApplication>
#include <QByteArray>
#include <QDoubleSpinBox>
#include <QMetaObject>
#include <QSpinBox>
#include <QString>
#include <QUndoStack>

#include <cassert>
#include <cmath>
#include <vector>

#include "Core/CommandStack.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/ShapeBuilder.h"
#include "GeometryKernel/Vector3.h"
#include "Scene/Document.h"
#include "ui/InspectorPanel.h"

namespace {

constexpr float kTolerance = 1e-3f;

GeometryObject* createCircle(Scene::Document& document, float radius, int segments)
{
    auto points = ShapeBuilder::buildCircle(Vector3(0.0f, 0.0f, 0.0f), Vector3(radius, 0.0f, 0.0f), segments);
    GeometryKernel& kernel = document.geometry();
    GeometryObject* object = kernel.addCurve(points);
    if (!object)
        return nullptr;

    GeometryKernel::ShapeMetadata metadata;
    metadata.type = GeometryKernel::ShapeMetadata::Type::Circle;
    metadata.circle.center = Vector3(0.0f, 0.0f, 0.0f);
    metadata.circle.direction = Vector3(1.0f, 0.0f, 0.0f);
    metadata.circle.radius = radius;
    metadata.circle.segments = segments;
    kernel.setShapeMetadata(object, metadata);

    Scene::Document::ObjectId id = document.ensureObjectForGeometry(object, "Circle");
    if (id == 0)
        return nullptr;

    document.synchronizeWithGeometry();
    object->setSelected(true);
    return object;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    Scene::Document document;
    QUndoStack undoStack;
    Core::CommandStack commandStack(&undoStack);

    Core::CommandContext context;
    context.document = &document;
    context.geometry = &document.geometry();
    context.geometryChanged = [&]() {
        document.synchronizeWithGeometry();
    };
    context.selectionChanged = [&](const std::vector<Scene::Document::ObjectId>& ids) {
        for (const auto& object : document.geometry().getObjects())
            object->setSelected(false);
        for (Scene::Document::ObjectId id : ids) {
            if (GeometryObject* geometry = document.geometryForObject(id))
                geometry->setSelected(true);
        }
    };
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

