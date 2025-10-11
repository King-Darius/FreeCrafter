#include <QApplication>
#include <QByteArray>
#include <QUndoStack>

#include <cassert>
#include <vector>

#include "CameraController.h"
#include "Core/CommandStack.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Vector3.h"
#include "Scene/Document.h"
#include "Tools/ChamferTool.h"
#include "Tools/LoftTool.h"

namespace {
std::vector<Vector3> makeRectangle(float width, float depth)
{
    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    return {
        { -hw, 0.0f, -hd },
        { hw, 0.0f, -hd },
        { hw, 0.0f, hd },
        { -hw, 0.0f, hd }
    };
}
}

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
    commandStack.setContext(context);

    CameraController camera;

    // Chamfer tool exercise
    Curve* chamferCurve = static_cast<Curve*>(document.geometry().addCurve(makeRectangle(2.0f, 2.0f)));
    assert(chamferCurve);
    document.synchronizeWithGeometry();
    chamferCurve->setSelected(true);

    ChamferTool chamfer(&document.geometry(), &camera);
    chamfer.setDocument(&document);
    chamfer.setCommandStack(&commandStack);
    chamfer.setViewportSize(800, 600);

    Phase6::RoundCornerOptions chamferOptions = chamfer.roundCornerOptions();
    chamferOptions.radius = 0.25f;
    chamferOptions.segments = 4;
    chamferOptions.style = Phase6::CornerStyle::Chamfer;
    chamferOptions.tagHardEdges = true;
    chamfer.setRoundCornerOptions(chamferOptions);

    Tool::PointerInput pointer{};
    chamfer.handleMouseDown(pointer);
    chamfer.handleMouseUp(pointer);

    document.synchronizeWithGeometry();
    chamferCurve = static_cast<Curve*>(document.geometry().getObjects().front().get());
    assert(chamferCurve);
    assert(chamferCurve->getBoundaryLoop().size() > 4);

    undoStack.undo();
    document.synchronizeWithGeometry();
    chamferCurve = static_cast<Curve*>(document.geometry().getObjects().front().get());
    assert(chamferCurve);
    assert(chamferCurve->getBoundaryLoop().size() == 4);

    undoStack.redo();
    document.synchronizeWithGeometry();
    chamferCurve = static_cast<Curve*>(document.geometry().getObjects().front().get());
    assert(chamferCurve);
    assert(chamferCurve->getBoundaryLoop().size() > 4);

    // Loft tool exercise
    Curve* base = static_cast<Curve*>(document.geometry().addCurve(makeRectangle(2.0f, 1.5f)));
    assert(base);
    std::vector<Vector3> elevated = makeRectangle(1.0f, 0.8f);
    for (auto& p : elevated)
        p.y = 1.2f;
    Curve* top = static_cast<Curve*>(document.geometry().addCurve(elevated));
    assert(top);
    document.synchronizeWithGeometry();

    base->setSelected(true);
    top->setSelected(true);

    std::size_t initialCount = document.geometry().getObjects().size();

    LoftTool loft(&document.geometry(), &camera);
    loft.setDocument(&document);
    loft.setCommandStack(&commandStack);
    loft.setViewportSize(800, 600);
    Phase6::LoftOptions loftOptions = loft.loftOptions();
    loftOptions.sections = 4;
    loftOptions.twistDegrees = 10.0f;
    loft.setLoftOptions(loftOptions);

    loft.handleMouseDown(pointer);
    loft.handleMouseUp(pointer);

    document.synchronizeWithGeometry();
    std::size_t afterLoftCount = document.geometry().getObjects().size();
    assert(afterLoftCount == initialCount + 1);

    undoStack.undo();
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().size() == initialCount);

    undoStack.redo();
    document.synchronizeWithGeometry();
    assert(document.geometry().getObjects().size() == initialCount + 1);

    return 0;
}
