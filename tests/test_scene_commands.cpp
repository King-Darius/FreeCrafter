#include <QApplication>
#include <QByteArray>
#include <QUndoStack>

#include <cassert>

#include "Scene/Document.h"
#include "Scene/SceneCommands.h"
#include "GeometryKernel/Vector3.h"

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    Scene::Document document;
    QUndoStack stack;

    // Primitive insertion smoke test
    Scene::PrimitiveOptions primitive;
    primitive.type = Scene::PrimitiveType::Box;
    primitive.width = 2.0f;
    primitive.depth = 2.0f;
    primitive.height = 2.0f;

    auto* addPrimitive = new Scene::AddPrimitiveCommand(document, primitive);
    stack.push(addPrimitive);
    assert(addPrimitive->error().empty());
    assert(!document.geometry().getObjects().empty());

    stack.undo();
    assert(document.geometry().getObjects().empty());

    // Guide state smoke test
    GeometryKernel::GuideState guides;
    guides.lines.push_back({ Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) });
    auto* setGuides = new Scene::SetGuidesCommand(document, guides);
    stack.push(setGuides);
    assert(document.geometry().getGuides().lines.size() == 1);
    stack.undo();
    assert(document.geometry().getGuides().lines.empty());

    // Image plane smoke test
    Scene::ImagePlaneOptions imageOptions;
    imageOptions.filePath = "test.png";
    imageOptions.width = 3.0f;
    imageOptions.height = 2.0f;
    auto* addImage = new Scene::AddImagePlaneCommand(document, imageOptions);
    stack.push(addImage);
    assert(addImage->error().empty());
    assert(!document.imagePlanes().empty());
    stack.undo();
    assert(document.imagePlanes().empty());

    // External reference smoke test
    Scene::ExternalReferenceOptions refOptions;
    refOptions.filePath = "model.obj";
    refOptions.displayName = "Model";
    refOptions.width = 5.0f;
    refOptions.depth = 3.0f;
    refOptions.height = 2.0f;
    auto* linkReference = new Scene::LinkExternalReferenceCommand(document, refOptions);
    stack.push(linkReference);
    assert(linkReference->error().empty());
    assert(!document.externalReferences().empty());
    stack.undo();
    assert(document.externalReferences().empty());

    return 0;
}

