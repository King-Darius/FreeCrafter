#include <cassert>
#include <cmath>
#include <vector>

#include "CameraController.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Vector3.h"
#include "Interaction/InferenceEngine.h"
#include "Tools/LineTool.h"
#include "Tools/SmartSelectTool.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Tools/ToolGeometryUtils.h"

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

int main()
{
    exerciseLineTool();
    exerciseSmartSelect();
    exerciseMoveTool();
    exerciseRotateTool();
    exerciseScaleTool();
    return 0;
}

