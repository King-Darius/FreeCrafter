#include <cassert>
#include <cmath>
#include <vector>

#include "CameraController.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"
#include "Tools/DrawingTools.h"
#include "Tools/ModificationTools.h"

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

void testArcTool()
{
    GeometryKernel kernel;
    CameraController camera;
    ArcTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(1.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() > 3);
}

void testCircleTool()
{
    GeometryKernel kernel;
    CameraController camera;
    CircleTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() == 32);
}

void testPolygonTool()
{
    GeometryKernel kernel;
    CameraController camera;
    PolygonTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);
    tool.setSides(5);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() == 5);
}

void testRotatedRectangleTool()
{
    GeometryKernel kernel;
    CameraController camera;
    RotatedRectangleTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() == 4);
}

void testFreehandTool()
{
    GeometryKernel kernel;
    CameraController camera;
    FreehandTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    Tool::PointerInput move{ 10, 0, {} };
    snap.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseMove(move);

    Tool::PointerInput move2{ 20, 0, {} };
    snap.position = Vector3(2.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseMove(move2);

    Tool::PointerInput up{ 20, 0, {} };
    tool.setInferenceResult(snap);
    tool.handleMouseUp(up);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() >= 3);
}

void testOffsetTool()
{
    GeometryKernel kernel;
    CameraController camera;
    GeometryObject* original = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    original->setSelected(true);

    OffsetTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);
    tool.setDistance(0.5f);
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getObjects().size() == 2);
    auto* curve = static_cast<Curve*>(kernel.getObjects().back().get());
    assert(curve->getBoundaryLoop().size() == 4);
}

void testPushPullTool()
{
    GeometryKernel kernel;
    CameraController camera;
    GeometryObject* original = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    original->setSelected(true);

    PushPullTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);
    tool.setDistance(3.0f);
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getObjects().size() == 2);
    assert(kernel.getObjects().back()->getType() == ObjectType::Solid);
}

void testFollowMeTool()
{
    GeometryKernel kernel;
    CameraController camera;
    GeometryObject* profile = kernel.addCurve(makeSquare(0.0f, 0.0f, 1.0f));
    profile->setSelected(true);
    GeometryObject* path = kernel.addCurve({ { 0.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 2.0f } });

    FollowMeTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);
    tool.setProfile(profile);
    tool.setPath(path);
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getObjects().size() >= 3);
}

void testPaintBucketTool()
{
    GeometryKernel kernel;
    CameraController camera;
    GeometryObject* curve = kernel.addCurve(makeSquare(0.0f, 0.0f, 1.0f));
    curve->setSelected(true);

    PaintBucketTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);
    tool.setMaterialName("Stone");
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getMaterial(curve) == "Stone");
}

void testTextTool()
{
    GeometryKernel kernel;
    CameraController camera;
    TextTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);
    tool.setText("Hello");
    tool.setHeight(2.0f);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;
    snap.position = Vector3(1.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getTextAnnotations().size() == 1);
    assert(std::fabs(kernel.getTextAnnotations().front().position.x - 1.0f) < 1e-5f);
}

void testDimensionTool()
{
    GeometryKernel kernel;
    CameraController camera;
    DimensionTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(3.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getDimensions().size() == 1);
    assert(std::fabs(kernel.getDimensions().front().value - 3.0f) < 1e-4f);
}

void testTapeMeasureTool()
{
    GeometryKernel kernel;
    CameraController camera;
    TapeMeasureTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(0.0f, 0.0f, 2.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getGuides().lines.size() == 1);
}

void testProtractorTool()
{
    GeometryKernel kernel;
    CameraController camera;
    ProtractorTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(0.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getGuides().angles.size() == 1);
    assert(std::fabs(kernel.getGuides().angles.front().angleDegrees - 90.0f) < 1e-3f);
}

void testAxesTool()
{
    GeometryKernel kernel;
    CameraController camera;
    AxesTool tool(&kernel, &camera);
    tool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(1.0f, 0.0f, 0.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(0.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    const auto& axes = kernel.getAxesState();
    assert(std::fabs(axes.origin.x - 0.0f) < 1e-5f);
    assert(std::fabs(axes.xAxis.x - 1.0f) < 1e-5f);
    assert(std::fabs(axes.yAxis.z - 1.0f) < 1e-5f);
}

int main()
{
    testArcTool();
    testCircleTool();
    testPolygonTool();
    testRotatedRectangleTool();
    testFreehandTool();
    testOffsetTool();
    testPushPullTool();
    testFollowMeTool();
    testPaintBucketTool();
    testTextTool();
    testDimensionTool();
    testTapeMeasureTool();
    testProtractorTool();
    testAxesTool();
    return 0;
}

