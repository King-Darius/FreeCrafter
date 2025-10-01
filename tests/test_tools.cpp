#include <cassert>
#include <cmath>
#include <vector>

#include "CameraController.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"
#include "GeometryKernel/Vector3.h"
#include "Interaction/InferenceEngine.h"
#include "Tools/LineTool.h"
#include "Tools/ArcTool.h"
#include "Tools/CircleTool.h"
#include "Tools/PolygonTool.h"
#include "Tools/RotatedRectangleTool.h"
#include "Tools/FreehandTool.h"
#include "Tools/SmartSelectTool.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Tools/ToolGeometryUtils.h"
#include "Tools/ExtrudeTool.h"
#include "Tools/OffsetTool.h"
#include "Tools/FollowMeTool.h"
#include "Tools/PaintBucketTool.h"
#include "Tools/TextTool.h"
#include "Tools/DimensionTool.h"
#include "Tools/TapeMeasureTool.h"
#include "Tools/ProtractorTool.h"
#include "Tools/AxesTool.h"
#include "Scene/Document.h"

using Interaction::InferenceResult;
using Interaction::InferenceSnapType;

std::vector<Vector3> makeSquare(float centerX, float centerZ, float size)
{
    float h = size * 0.5f;
    return {
        { centerX - h, 0.0f, centerZ - h },
        { centerX + h, 0.0f, centerZ - h },
        { centerX + h, 0.0f, centerZ + h },
        { centerX - h, 0.0f, centerZ + h }
    };
}

void exerciseLineTool()
{
    GeometryKernel kernel;
    CameraController camera;
    LineTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 100, 100, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::Endpoint;

    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(1.0f, 0.0f, 1.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    tool.commit();

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    const auto& loop = curve->getBoundaryLoop();
    assert(loop.size() == 3);
    assert(std::fabs(loop[2].z - 1.0f) < 1e-5f);
}

void exerciseSmartSelect()
{
    GeometryKernel kernel;
    CameraController camera;

    kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    kernel.addCurve(makeSquare(5.0f, 0.0f, 2.0f));

    SmartSelectTool select(&kernel, &camera);
    select.setViewportSize(800, 600);

    Tool::PointerInput down{ 10, 10, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::OnFace;
    inference.position = Vector3(-2.0f, 0.0f, -2.0f);
    select.setInferenceResult(inference);
    select.handleMouseDown(down);

    Tool::PointerInput move{ 20, 20, {} };
    inference.position = Vector3(2.0f, 0.0f, 2.0f);
    select.setInferenceResult(inference);
    select.handleMouseMove(move);

    Tool::PointerInput up{ 20, 20, {} };
    select.setInferenceResult(inference);
    select.handleMouseUp(up);

    assert(kernel.getObjects().front()->isSelected());
    assert(!kernel.getObjects().back()->isSelected());

    kernel.getObjects().front()->setSelected(false);
    kernel.getObjects().back()->setSelected(false);

    Tool::PointerInput downCross{ 40, 40, {} };
    inference.position = Vector3(6.0f, 0.0f, 6.0f);
    select.setInferenceResult(inference);
    select.handleMouseDown(downCross);

    Tool::PointerInput moveCross{ 10, 10, {} };
    inference.position = Vector3(-2.0f, 0.0f, -2.0f);
    select.setInferenceResult(inference);
    select.handleMouseMove(moveCross);

    Tool::PointerInput upCross{ 10, 10, {} };
    select.setInferenceResult(inference);
    select.handleMouseUp(upCross);

    assert(kernel.getObjects().front()->isSelected());
    assert(kernel.getObjects().back()->isSelected());
}

void exerciseMoveTool()
{
    GeometryKernel kernel;
    CameraController camera;
    GeometryObject* obj = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    obj->setSelected(true);

    MoveTool move(&kernel, &camera);
    move.setViewportSize(800, 600);

    Tool::PointerInput down{ 10, 10, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::Endpoint;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    move.setInferenceResult(inference);
    move.handleMouseDown(down);

    Tool::PointerInput moveInput{ 20, 10, {} };
    inference.position = Vector3(2.0f, 0.0f, 0.0f);
    move.setInferenceResult(inference);
    move.handleMouseMove(moveInput);

    Tool::PointerInput up{ 20, 10, {} };
    move.setInferenceResult(inference);
    move.handleMouseUp(up);

    auto* curve = static_cast<Curve*>(obj);
    const auto& loop = curve->getBoundaryLoop();
    assert(std::fabs(loop[0].x - 1.0f) < 1e-5f);
}

void exerciseRotateTool()
{
    GeometryKernel kernel;
    CameraController camera;
    GeometryObject* obj = kernel.addCurve({ {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} });
    obj->setSelected(true);

    RotateTool rotate(&kernel, &camera);
    rotate.setViewportSize(800, 600);

    Tool::PointerInput down{ 15, 15, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::Axis;
    inference.direction = Vector3(0.0f, 1.0f, 0.0f);
    inference.position = Vector3(1.0f, 0.0f, 0.0f);
    rotate.setInferenceResult(inference);
    rotate.handleMouseDown(down);

    Tool::PointerInput moveInput{ 25, 25, {} };
    inference.position = Vector3(0.0f, 0.0f, 1.0f);
    rotate.setInferenceResult(inference);
    rotate.handleMouseMove(moveInput);

    Tool::PointerInput up{ 25, 25, {} };
    rotate.setInferenceResult(inference);
    rotate.handleMouseUp(up);

    auto* curve = static_cast<Curve*>(obj);
    const auto& loop = curve->getBoundaryLoop();
    assert(loop.size() == 3);
    Vector3 pivot = computeCentroid(*obj);
    Vector3 relative = loop[0] - pivot;
    assert(std::fabs(relative.x) < 1e-4f);
    assert(std::fabs(relative.z + 1.0f) < 1e-4f);
}

void exerciseScaleTool()
{
    GeometryKernel kernel;
    CameraController camera;
    GeometryObject* obj = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    obj->setSelected(true);

    ScaleTool scale(&kernel, &camera);
    scale.setViewportSize(800, 600);

    Tool::PointerInput down{ 12, 12, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::Endpoint;
    inference.position = Vector3(1.0f, 0.0f, 0.0f);
    scale.setInferenceResult(inference);
    scale.handleMouseDown(down);

    Tool::PointerInput moveInput{ 22, 12, {} };
    inference.position = Vector3(2.0f, 0.0f, 0.0f);
    scale.setInferenceResult(inference);
    scale.handleMouseMove(moveInput);

    Tool::PointerInput up{ 22, 12, {} };
    scale.setInferenceResult(inference);
    scale.handleMouseUp(up);

    auto* curve = static_cast<Curve*>(obj);
    const auto& loop = curve->getBoundaryLoop();
    Vector3 pivot = computeCentroid(*obj);
    assert(std::fabs(loop[1].x - pivot.x - 2.0f) < 1e-5f);
    assert(std::fabs(loop[0].x - pivot.x + 2.0f) < 1e-5f);
}

void exerciseArcTool()
{
    GeometryKernel kernel;
    CameraController camera;
    ArcTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(std::sqrt(0.5f), 0.0f, std::sqrt(0.5f));
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(0.0f, 0.0f, 1.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    const auto& loop = curve->getBoundaryLoop();
    assert(loop.size() >= 3);
    assert(std::fabs(loop.front().x - 1.0f) < 1e-4f);
    assert(std::fabs(loop.back().z - 1.0f) < 1e-4f);
}

void exerciseCircleTool()
{
    GeometryKernel kernel;
    CameraController camera;
    CircleTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(2.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    const auto& loop = curve->getBoundaryLoop();
    assert(loop.size() == 64);
    float radius = (loop.front() - Vector3(0.0f, 0.0f, 0.0f)).length();
    assert(std::fabs(radius - 2.0f) < 1e-3f);
}

void exercisePolygonTool()
{
    GeometryKernel kernel;
    CameraController camera;
    PolygonTool tool(&kernel, &camera, 5);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(0.0f, 0.0f, 3.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    const auto& loop = curve->getBoundaryLoop();
    assert(loop.size() == 5);
}

void exerciseRotatedRectangleTool()
{
    GeometryKernel kernel;
    CameraController camera;
    RotatedRectangleTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(4.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(4.0f, 0.0f, 2.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    const auto& loop = curve->getBoundaryLoop();
    assert(loop.size() == 4);
    assert(std::fabs(loop[1].x - loop[0].x - 4.0f) < 1e-4f);
}

void exerciseFreehandTool()
{
    GeometryKernel kernel;
    CameraController camera;
    FreehandTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput down{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(down);

    Tool::PointerInput move1{ 10, 0, {} };
    inference.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseMove(move1);

    Tool::PointerInput move2{ 20, 0, {} };
    inference.position = Vector3(2.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseMove(move2);

    Tool::PointerInput up{ 20, 0, {} };
    tool.setInferenceResult(inference);
    tool.handleMouseUp(up);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    const auto& loop = curve->getBoundaryLoop();
    assert(loop.size() >= 3);
}

void exerciseExtrudeTool()
{
    GeometryKernel kernel;
    CameraController camera;
    ExtrudeTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    GeometryObject* base = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    assert(base);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::OnFace;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);
    tool.applyMeasurementOverride(3.0);
    tool.commit();

    assert(kernel.getObjects().size() == 2);
    auto* solid = dynamic_cast<Solid*>(kernel.getObjects().back().get());
    assert(solid);
    assert(std::fabs(solid->getHeight() - 3.0f) < 1e-5f);
}

void exerciseOffsetTool()
{
    GeometryKernel kernel;
    CameraController camera;
    OffsetTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    GeometryObject* curveObj = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    assert(curveObj);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::OnFace;
    inference.position = Vector3(0.25f, 0.0f, 0.25f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);
    tool.applyMeasurementOverride(0.5);
    tool.commit();

    assert(kernel.getObjects().size() == 2);
    auto* newCurve = dynamic_cast<Curve*>(kernel.getObjects().back().get());
    assert(newCurve);
    const auto& loop = newCurve->getBoundaryLoop();
    assert(loop.size() == 4);
    float span = loop[1].x - loop[0].x;
    assert(std::fabs(span - 3.0f) < 1e-4f);
}

void exerciseFollowMeTool()
{
    GeometryKernel kernel;
    CameraController camera;
    FollowMeTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    auto* profile = dynamic_cast<Curve*>(kernel.addCurve(makeSquare(0.0f, 0.0f, 1.0f)));
    auto* path = dynamic_cast<Curve*>(kernel.addCurve(makeSquare(4.0f, 0.0f, 4.0f)));
    assert(profile && path);
    profile->setSelected(true);
    path->setSelected(true);

    Tool::PointerInput input{ 0, 0, {} };
    tool.handleMouseDown(input);

    int solids = 0;
    for (const auto& object : kernel.getObjects()) {
        if (object->getType() == ObjectType::Solid)
            ++solids;
    }
    assert(solids == 4);
}

void exercisePaintBucketTool()
{
    GeometryKernel kernel;
    CameraController camera;
    PaintBucketTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    auto* curve = dynamic_cast<Curve*>(kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f)));
    assert(curve);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::OnFace;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    const Vector3& color = curve->getColor();
    assert(color.x > 0.5f && color.y < 0.5f);
}

void exerciseTextTool()
{
    Scene::Document document;
    CameraController camera;
    TextTool tool(&document.geometry(), &camera, &document);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(1.0f, 0.0f, 1.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(document.textAnnotations().size() == 1);
    assert(document.textAnnotations().front().position.x == 1.0f);
}

void exerciseDimensionTool()
{
    Scene::Document document;
    CameraController camera;
    DimensionTool tool(&document.geometry(), &camera, &document);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(3.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(document.linearDimensions().size() == 1);
    assert(std::fabs(document.linearDimensions().front().value - 3.0f) < 1e-5f);
}

void exerciseTapeMeasureTool()
{
    Scene::Document document;
    CameraController camera;
    TapeMeasureTool tool(&document.geometry(), &camera, &document);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(-1.0f, 0.0f, -1.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(2.0f, 0.0f, 2.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(document.guideLines().size() == 1);
    assert(document.guideLines().front().length > 0.0f);
}

void exerciseProtractorTool()
{
    Scene::Document document;
    CameraController camera;
    ProtractorTool tool(&document.geometry(), &camera, &document);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    inference.position = Vector3(0.0f, 0.0f, 1.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(document.angleGuides().size() == 1);
    assert(std::fabs(document.angleGuides().front().angle - 90.0f) < 1e-3f);
}

void exerciseAxesTool()
{
    Scene::Document document;
    CameraController camera;
    AxesTool tool(&document.geometry(), &camera, &document);
    tool.setViewportSize(800, 600);

    Tool::PointerInput input{ 0, 0, {} };
    InferenceResult inference;
    inference.type = InferenceSnapType::Axis;
    inference.direction = Vector3(0.0f, 0.0f, 1.0f);
    inference.position = Vector3(2.0f, 0.0f, 2.0f);
    tool.setInferenceResult(inference);
    tool.handleMouseDown(input);

    assert(document.axesGuides().size() == 1);
    const auto& guide = document.axesGuides().front();
    assert(std::fabs(guide.origin.x - 2.0f) < 1e-5f);
}

int main()
{
    exerciseLineTool();
    exerciseSmartSelect();
    exerciseMoveTool();
    exerciseRotateTool();
    exerciseScaleTool();
    exerciseArcTool();
    exerciseCircleTool();
    exercisePolygonTool();
    exerciseRotatedRectangleTool();
    exerciseFreehandTool();
    exerciseExtrudeTool();
    exerciseOffsetTool();
    exerciseFollowMeTool();
    exercisePaintBucketTool();
    exerciseTextTool();
    exerciseDimensionTool();
    exerciseTapeMeasureTool();
    exerciseProtractorTool();
    exerciseAxesTool();
    return 0;
}

