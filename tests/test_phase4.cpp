#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "CameraController.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"
#include "Tools/DrawingTools.h"
#include "Tools/ModificationTools.h"
#include "test_support.h"

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

void testCenterArcTool()
{
    GeometryKernel kernel;
    CameraController camera;
    CenterArcTool tool(&kernel, &camera);
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

    snap.position = Vector3(0.0f, 0.0f, 2.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() > 3);
    auto metadata = kernel.shapeMetadata(curve);
    assert(metadata.has_value());
    assert(metadata->type == GeometryKernel::ShapeMetadata::Type::Arc);
    assert(std::fabs(metadata->arc.definition.radius - 2.0f) < 1e-3f);
}

void testTangentArcTool()
{
    GeometryKernel kernel;
    CameraController camera;
    TangentArcTool tool(&kernel, &camera);
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

    snap.position = Vector3(1.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() > 3);
    auto metadata = kernel.shapeMetadata(curve);
    assert(metadata.has_value());
    assert(metadata->type == GeometryKernel::ShapeMetadata::Type::Arc);
    assert(metadata->arc.definition.radius > 0.0f);
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

void testBezierTool()
{
    GeometryKernel kernel;
    CameraController camera;
    BezierTool tool(&kernel, &camera);
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

    snap.position = Vector3(0.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 1.0f);
    tool.setInferenceResult(snap);
    tool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    auto* curve = static_cast<Curve*>(kernel.getObjects().front().get());
    assert(curve->getBoundaryLoop().size() > 3);
    auto metadata = kernel.shapeMetadata(curve);
    assert(metadata.has_value());
    assert(metadata->type == GeometryKernel::ShapeMetadata::Type::Bezier);
    assert(metadata->bezier.definition.segments >= 16);
}

void testOffsetTool()
{
    TestSupport::ToolCommandHarness harness;
    CameraController camera;
    GeometryKernel& kernel = harness.geometry;
    GeometryObject* original = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    harness.registerObject(original, "Offset Source");
    original->setSelected(true);

    OffsetTool tool(&kernel, &camera);
    harness.configureTool(tool);
    tool.setViewportSize(800, 600);
    tool.setDistance(0.5f);
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getObjects().size() == 2);
    auto* curve = static_cast<Curve*>(kernel.getObjects().back().get());
    assert(curve->getBoundaryLoop().size() == 4);
}

void testPushPullTool()
{
    TestSupport::ToolCommandHarness harness;
    CameraController camera;
    GeometryKernel& kernel = harness.geometry;
    GeometryObject* original = kernel.addCurve(makeSquare(0.0f, 0.0f, 2.0f));
    harness.registerObject(original, "PushPull Source");
    original->setSelected(true);

    PushPullTool tool(&kernel, &camera);
    harness.configureTool(tool);
    tool.setViewportSize(800, 600);
    tool.setDistance(3.0f);
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getObjects().size() == 2);
    assert(kernel.getObjects().back()->getType() == ObjectType::Solid);
}

void testFollowMeTool()
{
    TestSupport::ToolCommandHarness harness;
    CameraController camera;
    GeometryKernel& kernel = harness.geometry;
    GeometryObject* profile = kernel.addCurve(makeSquare(0.0f, 0.0f, 1.0f));
    harness.registerObject(profile, "FollowMe Profile");
    profile->setSelected(true);
    GeometryObject* path = kernel.addCurve({ { 0.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 2.0f } });
    harness.registerObject(path, "FollowMe Path");

    FollowMeTool tool(&kernel, &camera);
    harness.configureTool(tool);
    tool.setViewportSize(800, 600);
    tool.setProfile(profile);
    tool.setPath(path);
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getObjects().size() >= 3);
}

void testPaintBucketTool()
{
    TestSupport::ToolCommandHarness harness;
    CameraController camera;
    GeometryKernel& kernel = harness.geometry;
    GeometryObject* curve = kernel.addCurve(makeSquare(0.0f, 0.0f, 1.0f));
    harness.registerObject(curve, "Paint Target");
    curve->setSelected(true);

    PaintBucketTool tool(&kernel, &camera);
    harness.configureTool(tool);
    tool.setViewportSize(800, 600);
    tool.setMaterialName("Stone");
    tool.handleMouseDown({ 0, 0, {} });

    assert(kernel.getMaterial(curve) == "Stone");
}

void testTextTool()
{
    TestSupport::ToolCommandHarness harness;
    CameraController camera;
    GeometryKernel& kernel = harness.geometry;
    TextTool tool(&kernel, &camera);
    harness.configureTool(tool);
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

void testShapeMetadataRebuild()
{
    GeometryKernel kernel;
    CameraController camera;
    CircleTool circle(&kernel, &camera);
    circle.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    circle.setInferenceResult(snap);
    circle.handleMouseDown(click);

    snap.position = Vector3(1.0f, 0.0f, 0.0f);
    circle.setInferenceResult(snap);
    circle.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    GeometryObject* object = kernel.getObjects().front().get();
    auto metadata = kernel.shapeMetadata(object);
    assert(metadata.has_value());
    metadata->circle.radius = 2.5f;
    metadata->circle.segments = 48;
    bool rebuilt = kernel.rebuildShapeFromMetadata(object, *metadata);
    assert(rebuilt);
    auto refreshed = kernel.shapeMetadata(object);
    assert(refreshed.has_value());
    assert(std::fabs(refreshed->circle.radius - 2.5f) < 1e-3f);
    assert(refreshed->circle.segments == 48);
    auto* curve = static_cast<Curve*>(object);
    assert(curve->getBoundaryLoop().size() == 48);
}

void testPolygonMetadataRebuild()
{
    GeometryKernel kernel;
    CameraController camera;
    PolygonTool polygon(&kernel, &camera);
    polygon.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    polygon.setInferenceResult(snap);
    polygon.handleMouseDown(click);

    snap.position = Vector3(1.0f, 0.0f, 0.0f);
    polygon.setInferenceResult(snap);
    polygon.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    GeometryObject* object = kernel.getObjects().front().get();
    auto metadata = kernel.shapeMetadata(object);
    assert(metadata.has_value());
    metadata->polygon.radius = 2.0f;
    metadata->polygon.sides = 9;
    bool rebuilt = kernel.rebuildShapeFromMetadata(object, *metadata);
    assert(rebuilt);
    auto refreshed = kernel.shapeMetadata(object);
    assert(refreshed.has_value());
    assert(std::fabs(refreshed->polygon.radius - 2.0f) < 1e-3f);
    assert(refreshed->polygon.sides == 9);
    auto* curve = static_cast<Curve*>(object);
    assert(curve->getBoundaryLoop().size() == 9);
}

void testArcMetadataRebuild()
{
    GeometryKernel kernel;
    CameraController camera;
    CenterArcTool arcTool(&kernel, &camera);
    arcTool.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    arcTool.setInferenceResult(snap);
    arcTool.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 0.0f);
    arcTool.setInferenceResult(snap);
    arcTool.handleMouseDown(click);

    snap.position = Vector3(0.0f, 0.0f, 2.0f);
    arcTool.setInferenceResult(snap);
    arcTool.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    GeometryObject* object = kernel.getObjects().front().get();
    auto metadata = kernel.shapeMetadata(object);
    assert(metadata.has_value());
    metadata->arc.definition.radius = 3.0f;
    metadata->arc.definition.startAngle = 0.0f;
    metadata->arc.definition.counterClockwise = true;
    metadata->arc.definition.endAngle = static_cast<float>(0.5 * 3.14159265358979323846);
    metadata->arc.definition.segments = 90;
    bool rebuilt = kernel.rebuildShapeFromMetadata(object, *metadata);
    assert(rebuilt);
    auto refreshed = kernel.shapeMetadata(object);
    assert(refreshed.has_value());
    assert(std::fabs(refreshed->arc.definition.radius - 3.0f) < 1e-3f);
    assert(refreshed->arc.definition.segments == 90);
    auto* curve = static_cast<Curve*>(object);
    assert(curve->getBoundaryLoop().size() == refreshed->arc.definition.segments + 1);
}

void testBezierMetadataRebuild()
{
    GeometryKernel kernel;
    CameraController camera;
    BezierTool bezier(&kernel, &camera);
    bezier.setViewportSize(800, 600);

    Tool::PointerInput click{ 0, 0, {} };
    InferenceResult snap;
    snap.type = InferenceSnapType::Endpoint;

    snap.position = Vector3(0.0f, 0.0f, 0.0f);
    bezier.setInferenceResult(snap);
    bezier.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 0.0f);
    bezier.setInferenceResult(snap);
    bezier.handleMouseDown(click);

    snap.position = Vector3(0.0f, 0.0f, 1.0f);
    bezier.setInferenceResult(snap);
    bezier.handleMouseDown(click);

    snap.position = Vector3(2.0f, 0.0f, 1.0f);
    bezier.setInferenceResult(snap);
    bezier.handleMouseDown(click);

    assert(kernel.getObjects().size() == 1);
    GeometryObject* object = kernel.getObjects().front().get();
    auto metadata = kernel.shapeMetadata(object);
    assert(metadata.has_value());
    metadata->bezier.definition.p0 = Vector3(-1.0f, 0.0f, 0.0f);
    metadata->bezier.definition.h0 = Vector3(0.0f, 0.0f, 2.0f);
    metadata->bezier.definition.h1 = Vector3(3.0f, 0.0f, -1.0f);
    metadata->bezier.definition.p1 = Vector3(1.5f, 0.0f, 1.5f);
    metadata->bezier.definition.segments = 64;
    bool rebuilt = kernel.rebuildShapeFromMetadata(object, *metadata);
    assert(rebuilt);
    auto refreshed = kernel.shapeMetadata(object);
    assert(refreshed.has_value());
    assert(refreshed->bezier.definition.segments == 64);
    auto* curve = static_cast<Curve*>(object);
    assert(curve->getBoundaryLoop().size() == refreshed->bezier.definition.segments + 1);
}

int main()
{
    testArcTool();
    testCenterArcTool();
    testTangentArcTool();
    testCircleTool();
    testPolygonTool();
    testRotatedRectangleTool();
    testFreehandTool();
    testBezierTool();
    testOffsetTool();
    testPushPullTool();
    testFollowMeTool();
    testPaintBucketTool();
    testTextTool();
    testDimensionTool();
    testTapeMeasureTool();
    testProtractorTool();
    testAxesTool();
    testShapeMetadataRebuild();
    testPolygonMetadataRebuild();
    testArcMetadataRebuild();
    testBezierMetadataRebuild();
    return 0;
}

