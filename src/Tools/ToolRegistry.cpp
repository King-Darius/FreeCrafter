#include "ToolRegistry.h"

#include "ChamferTool.h"
#include "DrawingTools.h"
#include "ExtrudeTool.h"
#include "LineTool.h"
#include "LoftTool.h"
#include "ModificationTools.h"
#include "MoveTool.h"
#include "OrbitTool.h"
#include "PanTool.h"
#include "RotateTool.h"
#include "ScaleTool.h"
#include "SectionTool.h"
#include "SmartSelectTool.h"
#include "ZoomTool.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QtGlobal>

#include <utility>

ToolRegistry::ToolDescriptor::ToolDescriptor(ToolId key,
    const char* idLiteral,
    const char* iconLiteral,
    const char* label,
    const char* statusTip,
    const char* hint,
    ToolFactory factoryFn,
    bool isNavigation)
    : key(key)
    , idLiteral(idLiteral)
    , id(idLiteral ? QString::fromLatin1(idLiteral) : QString())
    , iconPath(iconLiteral ? QString::fromUtf8(iconLiteral) : QString())
    , labelKey(label)
    , statusTipKey(statusTip)
    , hintKey(hint)
    , factory(std::move(factoryFn))
    , navigation(isNavigation)
{
}

const ToolRegistry& ToolRegistry::instance()
{
    static const ToolRegistry registry;
    return registry;
}

ToolRegistry::ToolRegistry()
{
    entries.reserve(static_cast<std::size_t>(ToolId::Zoom) + 1);

    auto add = [this](ToolDescriptor descriptor) {
        const std::size_t index = entries.size();
        entries.push_back(std::move(descriptor));
        indexById.insert(entries.back().id, index);
    };

    add(ToolDescriptor(ToolId::SmartSelect,
        "SmartSelectTool",
        ":/icons/select.png",
        QT_TR_NOOP("Select"),
        QT_TR_NOOP("Select and edit existing geometry."),
        QT_TR_NOOP("Select: Click to pick, drag left-to-right for window, right-to-left for crossing."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<SmartSelectTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Line,
        "LineTool",
        ":/icons/line.png",
        QT_TR_NOOP("Line"),
        QT_TR_NOOP("Draw connected line segments."),
        QT_TR_NOOP("Line: Click to place vertices. Press Enter to finish, Esc to cancel."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<LineTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Rectangle,
        "Rectangle",
        ":/icons/rectangle.png",
        QT_TR_NOOP("Rectangle"),
        QT_TR_NOOP("Create rectangles aligned to the axes."),
        QT_TR_NOOP("Rectangle: Click the first corner, then click the opposite corner to finish."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<RectangleTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Arc,
        "Arc",
        ":/icons/arc.png",
        QT_TR_NOOP("3-Point Arc"),
        QT_TR_NOOP("Draw arcs using three points."),
        QT_TR_NOOP("Arc: Click the first endpoint, click the second endpoint, then click to define the bulge."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<ArcTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::CenterArc,
        "CenterArc",
        ":/icons/center_arc.svg",
        QT_TR_NOOP("Center Arc"),
        QT_TR_NOOP("Draw arcs from a center, start, and end angle."),
        QT_TR_NOOP("Center Arc: Click the center, click the start angle, then click to set the end angle."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<CenterArcTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::TangentArc,
        "TangentArc",
        ":/icons/tangent_arc.svg",
        QT_TR_NOOP("Tangent Arc"),
        QT_TR_NOOP("Draw arcs tangent to a direction at the first point."),
        QT_TR_NOOP("Tangent Arc: Click the start point, click to set tangent direction, then click the end point."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<TangentArcTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Circle,
        "Circle",
        ":/icons/circle.png",
        QT_TR_NOOP("Circle"),
        QT_TR_NOOP("Draw circles by center and radius."),
        QT_TR_NOOP("Circle: Click to set the center, then click again to define the radius."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<CircleTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Polygon,
        "Polygon",
        ":/icons/polygon.svg",
        QT_TR_NOOP("Polygon"),
        QT_TR_NOOP("Draw regular polygons by center and radius."),
        QT_TR_NOOP("Polygon: Click to set the center, then click again to set the radius. Type a side count to change segments."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<PolygonTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::RotatedRectangle,
        "RotatedRectangle",
        ":/icons/rotated_rectangle.svg",
        QT_TR_NOOP("Rotated Rectangle"),
        QT_TR_NOOP("Draw rectangles by edge, rotation, and height."),
        QT_TR_NOOP("Rotated Rectangle: Click two corners for the base edge, then click to set height."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<RotatedRectangleTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Freehand,
        "Freehand",
        ":/icons/freehand.svg",
        QT_TR_NOOP("Freehand"),
        QT_TR_NOOP("Sketch loose strokes that become polylines."),
        QT_TR_NOOP("Freehand: Click and drag to sketch a polyline stroke."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<FreehandTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Bezier,
        "Bezier",
        ":/icons/bezier.svg",
        QT_TR_NOOP("Bezier"),
        QT_TR_NOOP("Create cubic Bezier curves with adjustable handles."),
        QT_TR_NOOP("Bezier: Click start and end anchors, then place the two handles."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<BezierTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Move,
        "MoveTool",
        ":/icons/move.png",
        QT_TR_NOOP("Move"),
        QT_TR_NOOP("Translate geometry to a new location."),
        QT_TR_NOOP("Move: Drag to translate selection. Use arrow keys for axis locks."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<MoveTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Rotate,
        "RotateTool",
        ":/icons/rotate.png",
        QT_TR_NOOP("Rotate"),
        QT_TR_NOOP("Rotate entities around a pivot."),
        QT_TR_NOOP("Rotate: Drag to rotate around pivot. Use snaps or axis locks."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<RotateTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Scale,
        "ScaleTool",
        ":/icons/scale.png",
        QT_TR_NOOP("Scale"),
        QT_TR_NOOP("Scale geometry about a point."),
        QT_TR_NOOP("Scale: Drag to scale selection. Axis locks limit scaling direction."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<ScaleTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Offset,
        "Offset",
        ":/icons/offset.png",
        QT_TR_NOOP("Offset"),
        QT_TR_NOOP("Offset edges to create parallel copies."),
        QT_TR_NOOP("Offset: Select a loop or edge chain, then drag to set the distance."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<OffsetTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::PushPull,
        "PushPull",
        ":/icons/pushpull.png",
        QT_TR_NOOP("Push/Pull"),
        QT_TR_NOOP("Push or pull faces to add volume."),
        QT_TR_NOOP("Push/Pull: Click a face and drag to add or remove volume."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<PushPullTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::FollowMe,
        "FollowMe",
        ":/icons/followme.png",
        QT_TR_NOOP("Follow Me"),
        QT_TR_NOOP("Sweep a profile along a path."),
        QT_TR_NOOP("Follow Me: Select a profile, then choose a path to sweep along."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<FollowMeTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::PaintBucket,
        "PaintBucket",
        ":/icons/paintbucket.png",
        QT_TR_NOOP("Paint"),
        QT_TR_NOOP("Apply materials to faces."),
        QT_TR_NOOP("Paint Bucket: Click a face to assign the active material."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<PaintBucketTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Text,
        "Text",
        ":/icons/text.png",
        QT_TR_NOOP("Text"),
        QT_TR_NOOP("Place 3D text."),
        QT_TR_NOOP("Text: Click to place 3D text oriented toward the camera."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<TextTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Dimension,
        "Dimension",
        ":/icons/measure.svg",
        QT_TR_NOOP("Dimension"),
        QT_TR_NOOP("Create linear dimensions."),
        QT_TR_NOOP("Dimension: Click two points to place a dimension label."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<DimensionTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::TapeMeasure,
        "TapeMeasure",
        ":/icons/tapemeasure.png",
        QT_TR_NOOP("Tape Measure"),
        QT_TR_NOOP("Measure distances in the model."),
        QT_TR_NOOP("Tape Measure: Click two points to sample distance."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<TapeMeasureTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Protractor,
        "Protractor",
        ":/icons/protractor.png",
        QT_TR_NOOP("Protractor"),
        QT_TR_NOOP("Measure angles in the model."),
        QT_TR_NOOP("Protractor: Click three points to measure an angle."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<ProtractorTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Axes,
        "Axes",
        ":/icons/axes.png",
        QT_TR_NOOP("Axes"),
        QT_TR_NOOP("Relocate the drawing axes."),
        QT_TR_NOOP("Axes: Click an origin and axes directions to position custom axes."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<AxesTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Extrude,
        "ExtrudeTool",
        ":/icons/pushpull.png",
        QT_TR_NOOP("Extrude"),
        QT_TR_NOOP("Push or pull faces to add volume."),
        QT_TR_NOOP("Extrude: Click to extrude last curve by 1.0."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<ExtrudeTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Chamfer,
        "ChamferTool",
        ":/icons/offset.png",
        QT_TR_NOOP("Chamfer"),
        QT_TR_NOOP("Apply chamfers or fillets to a selected curve."),
        QT_TR_NOOP("Chamfer: Select a closed curve, adjust radius, then click Apply."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<ChamferTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Loft,
        "LoftTool",
        ":/icons/followme.png",
        QT_TR_NOOP("Loft"),
        QT_TR_NOOP("Create a lofted solid between two selected profiles."),
        QT_TR_NOOP("Loft: Select two profiles, adjust sections, then click Apply."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<LoftTool>(ctx.geometry, ctx.camera);
        },
        false));

    add(ToolDescriptor(ToolId::Section,
        "SectionTool",
        ":/icons/section.png",
        QT_TR_NOOP("Section"),
        QT_TR_NOOP("Create section planes through the model."),
        QT_TR_NOOP("Section Plane: Click to place a cutting plane using current inference cues."),
        [](const ToolCreationContext& ctx) {
            return std::make_unique<SectionTool>(ctx.geometry, ctx.camera, ctx.document);
        },
        false));

    add(ToolDescriptor(ToolId::Orbit,
        "OrbitTool",
        ":/icons/orbit.svg",
        QT_TR_NOOP("Orbit"),
        QT_TR_NOOP("Orbit around the scene."),
        nullptr,
        [](const ToolCreationContext& ctx) {
            return std::make_unique<OrbitTool>(ctx.geometry, ctx.camera);
        },
        true));

    add(ToolDescriptor(ToolId::Pan,
        "PanTool",
        ":/icons/pan.svg",
        QT_TR_NOOP("Pan"),
        QT_TR_NOOP("Pan the camera view."),
        nullptr,
        [](const ToolCreationContext& ctx) {
            return std::make_unique<PanTool>(ctx.geometry, ctx.camera);
        },
        true));

    add(ToolDescriptor(ToolId::Zoom,
        "ZoomTool",
        ":/icons/zoom.png",
        QT_TR_NOOP("Zoom"),
        QT_TR_NOOP("Zoom the camera view."),
        nullptr,
        [](const ToolCreationContext& ctx) {
            return std::make_unique<ZoomTool>(ctx.geometry, ctx.camera);
        },
        true));

    Q_ASSERT(entries.size() == static_cast<std::size_t>(ToolId::Zoom) + 1);
}

const ToolRegistry::ToolDescriptor& ToolRegistry::descriptor(ToolId key) const
{
    const std::size_t index = static_cast<std::size_t>(key);
    Q_ASSERT(index < entries.size());
    return entries[index];
}

const ToolRegistry::ToolDescriptor* ToolRegistry::findById(const QString& id) const
{
    const auto it = indexById.constFind(id);
    if (it == indexById.constEnd())
        return nullptr;
    return &entries[*it];
}
