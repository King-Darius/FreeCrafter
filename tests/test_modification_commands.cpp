#include <QApplication>
#include <QByteArray>
#include <QUndoStack>

#include <cassert>
#include <vector>

#include "CameraController.h"
#include "Core/CommandStack.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/GeometryKernel.h"
#include "Interaction/InferenceEngine.h"
#include "Scene/Document.h"
#include "Tools/ModificationTools.h"

using Interaction::InferenceResult;
using Interaction::InferenceSnapType;

std::vector<Vector3> makeSquare(float centerX, float centerZ, float size)
{
    float half = size * 0.5f;
    return {
        { centerX - half, 0.0f, centerZ - half },
        { centerX + half, 0.0f, centerZ - half },
        { centerX + half, 0.0f, centerZ + half },
        { centerX - half, 0.0f, centerZ + half }
    };
}

std::vector<Scene::Document::ObjectId> gatherSelection(Scene::Document& document)
{
    std::vector<Scene::Document::ObjectId> ids;
    for (const auto& object : document.geometry().getObjects()) {
        if (!object->isSelected())
            continue;
        Scene::Document::ObjectId id = document.objectIdForGeometry(object.get());
        if (id != 0)
            ids.push_back(id);
    }
    return ids;
}

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    Scene::Document document;
    CameraController camera;
    QUndoStack undoStack;
    Core::CommandStack commandStack(&undoStack);

    Core::CommandContext context;
    context.document = &document;
    context.geometry = &document.geometry();
    context.geometryChanged = [&]() {
        document.synchronizeWithGeometry();
    };
    commandStack.setContext(context);

    Tool::PointerInput input{ 0, 0, {} };

    auto ensureClean = [&]() {
        undoStack.clear();
        document.reset();
        document.synchronizeWithGeometry();
    };

    // Offset tool
    ensureClean();
    GeometryObject* base = document.geometry().addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    assert(base);
    Scene::Document::ObjectId baseId = document.ensureObjectForGeometry(base, "Base");
    assert(baseId != 0);
    base->setSelected(true);

    OffsetTool offset(&document.geometry(), &camera);
    offset.setDocument(&document);
    offset.setCommandStack(&commandStack);
    offset.setDistance(0.5f);
    offset.handleMouseDown(input);
    offset.handleMouseUp(input);

    assert(document.geometry().getObjects().size() == 2);
    auto selection = gatherSelection(document);
    assert(selection.size() == 1);
    assert(selection.front() != baseId);

    undoStack.undo();
    assert(document.geometry().getObjects().size() == 1);
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() == baseId);

    undoStack.redo();
    assert(document.geometry().getObjects().size() == 2);
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() != baseId);

    // Push/Pull tool
    ensureClean();
    base = document.geometry().addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    assert(base);
    baseId = document.ensureObjectForGeometry(base, "Profile");
    assert(baseId != 0);
    base->setSelected(true);

    PushPullTool pushPull(&document.geometry(), &camera);
    pushPull.setDocument(&document);
    pushPull.setCommandStack(&commandStack);
    pushPull.setDistance(2.0f);
    pushPull.handleMouseDown(input);
    pushPull.handleMouseUp(input);

    assert(document.geometry().getObjects().size() == 2);
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() != baseId);

    undoStack.undo();
    assert(document.geometry().getObjects().size() == 1);
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() == baseId);

    undoStack.redo();
    assert(document.geometry().getObjects().size() == 2);
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() != baseId);

    // Follow Me tool
    ensureClean();
    GeometryObject* profile = document.geometry().addCurve(makeSquare(0.0f, 0.0f, 1.0f));
    assert(profile);
    Scene::Document::ObjectId profileId = document.ensureObjectForGeometry(profile, "Profile");
    assert(profileId != 0);
    GeometryObject* path = document.geometry().addCurve({ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, { 2.0f, 0.0f, 2.0f } });
    assert(path);
    Scene::Document::ObjectId pathId = document.ensureObjectForGeometry(path, "Path");
    assert(pathId != 0);
    profile->setSelected(true);

    FollowMeTool follow(&document.geometry(), &camera);
    follow.setDocument(&document);
    follow.setCommandStack(&commandStack);
    follow.setProfile(profile);
    follow.setPath(path);
    follow.handleMouseDown(input);
    follow.handleMouseUp(input);

    assert(document.geometry().getObjects().size() == 5);
    selection = gatherSelection(document);
    assert(!selection.empty());
    for (Scene::Document::ObjectId id : selection)
        assert(id != profileId && id != pathId);

    undoStack.undo();
    assert(document.geometry().getObjects().size() == 2);
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() == profileId);

    undoStack.redo();
    assert(document.geometry().getObjects().size() == 5);
    selection = gatherSelection(document);
    assert(!selection.empty());
    for (Scene::Document::ObjectId id : selection)
        assert(id != profileId && id != pathId);

    // Paint Bucket tool
    ensureClean();
    GeometryObject* paintTarget = document.geometry().addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    assert(paintTarget);
    Scene::Document::ObjectId paintId = document.ensureObjectForGeometry(paintTarget, "Paint");
    assert(paintId != 0);
    paintTarget->setSelected(true);

    PaintBucketTool paint(&document.geometry(), &camera);
    paint.setDocument(&document);
    paint.setCommandStack(&commandStack);
    paint.setMaterialName("Brick");
    paint.handleMouseDown(input);
    paint.handleMouseUp(input);

    assert(document.geometry().getMaterial(paintTarget) == "Brick");
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() == paintId);

    undoStack.undo();
    assert(document.geometry().getMaterial(paintTarget).empty());
    selection = gatherSelection(document);
    assert(selection.size() == 1 && selection.front() == paintId);

    undoStack.redo();
    assert(document.geometry().getMaterial(paintTarget) == "Brick");

    // Text tool
    ensureClean();
    TextTool textTool(&document.geometry(), &camera);
    textTool.setDocument(&document);
    textTool.setCommandStack(&commandStack);
    textTool.setText("Hello");
    textTool.setHeight(0.5f);

    InferenceResult inference;
    inference.type = InferenceSnapType::Endpoint;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    textTool.setInferenceResult(inference);
    textTool.handleMouseDown(input);
    textTool.handleMouseUp(input);

    assert(document.geometry().getTextAnnotations().size() == 1);
    undoStack.undo();
    assert(document.geometry().getTextAnnotations().empty());
    undoStack.redo();
    assert(document.geometry().getTextAnnotations().size() == 1);

    return 0;
}
