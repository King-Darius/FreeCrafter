#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QLineEdit>
#include <QMetaObject>
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

Vector3 computeCentroid(const GeometryObject& object)
{
    const auto& vertices = object.getMesh().getVertices();
    if (vertices.empty())
        return Vector3();
    Vector3 sum(0.0f, 0.0f, 0.0f);
    for (const auto& vertex : vertices)
        sum += vertex.position;
    return sum / static_cast<float>(vertices.size());
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
    Scene::Document::ObjectId circleId = document.objectIdForGeometry(circle);
    assert(circleId != 0);

    InspectorPanel panel;
    panel.setDocument(&document);
    panel.setCommandStack(&commandStack);

    std::vector<GeometryObject*> selection { circle };
    panel.updateSelection(&document.geometry(), selection);

    auto* nameEdit = panel.findChild<QLineEdit*>(QStringLiteral("inspectorNameEdit"));
    assert(nameEdit);
    nameEdit->setText(QStringLiteral("Renamed Circle"));
    QMetaObject::invokeMethod(&panel, "commitName", Qt::DirectConnection);

    const Scene::Document::ObjectNode* node = document.findObject(circleId);
    assert(node && node->name == "Renamed Circle");
    panel.updateSelection(&document.geometry(), selection);
    assert(nameEdit->text() == QStringLiteral("Renamed Circle"));

    undoStack.undo();
    node = document.findObject(circleId);
    assert(node && node->name == "Circle");
    panel.updateSelection(&document.geometry(), selection);
    assert(nameEdit->text() == QStringLiteral("Circle"));
    undoStack.redo();
    panel.updateSelection(&document.geometry(), selection);

    auto* visibilityCheck = panel.findChild<QCheckBox*>(QStringLiteral("inspectorVisibleCheck"));
    assert(visibilityCheck);
    visibilityCheck->setChecked(false);
    node = document.findObject(circleId);
    assert(node && !node->visible);
    undoStack.undo();
    node = document.findObject(circleId);
    assert(node && node->visible);
    undoStack.redo();
    panel.updateSelection(&document.geometry(), selection);

    Vector3 originalCenter = computeCentroid(*circle);
    auto* positionX = panel.findChild<QLineEdit*>(QStringLiteral("inspectorPositionX"));
    assert(positionX);
    double targetX = static_cast<double>(originalCenter.x) + 2.0;
    positionX->setText(QString::number(targetX, 'f', 3));
    QMetaObject::invokeMethod(&panel, "commitPositionX", Qt::DirectConnection);

    Vector3 movedCenter = computeCentroid(*circle);
    assert(std::fabs(movedCenter.x - static_cast<float>(targetX)) < kTolerance);
    undoStack.undo();
    Vector3 revertedCenter = computeCentroid(*circle);
    assert(std::fabs(revertedCenter.x - originalCenter.x) < kTolerance);
    undoStack.redo();

    return 0;
}
