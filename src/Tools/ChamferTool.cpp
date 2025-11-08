#include "ChamferTool.h"

#include "ToolCommands.h"

#include "../Core/CommandStack.h"
#include "../GeometryKernel/GeometryKernel.h"
#include "../Scene/Document.h"

#include <algorithm>
#include <memory>

namespace {
constexpr float kMinRadius = 1e-4f;
}

ChamferTool::ChamferTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
    options.radius = 0.1f;
    options.segments = 6;
    options.style = Phase6::CornerStyle::Chamfer;
    options.tagHardEdges = true;
}

Tool::OverrideResult ChamferTool::applyMeasurementOverride(double value)
{
    if (value <= 0.0)
        return Tool::OverrideResult::Ignored;
    options.radius = std::max(static_cast<float>(value), kMinRadius);
    if (rebuildPreview())
        return Tool::OverrideResult::PreviewUpdated;
    return Tool::OverrideResult::Ignored;
}

void ChamferTool::setRoundCornerOptions(const Phase6::RoundCornerOptions& opts)
{
    options = opts;
    options.radius = std::max(options.radius, kMinRadius);
    options.segments = std::max(1, options.segments);
    rebuildPreview();
}

bool ChamferTool::selectTarget()
{
    targetCurve = nullptr;
    targetId = 0;
    if (!geometry)
        return false;
    for (const auto& object : geometry->getObjects()) {
        if (!object || !object->isSelected())
            continue;
        if (object->getType() != ObjectType::Curve)
            continue;
        targetCurve = static_cast<Curve*>(object.get());
        if (Scene::Document* doc = getDocument())
            targetId = doc->objectIdForGeometry(object.get());
        break;
    }
    if (!targetCurve)
        return false;
    return !targetCurve->getBoundaryLoop().empty();
}

void ChamferTool::onPointerDown(const PointerInput&)
{
    if (!selectTarget())
        return;
    if (!rebuildPreview()) {
        resetState();
        return;
    }
    setState(State::Active);
}

void ChamferTool::onPointerUp(const PointerInput&)
{
    if (getState() != State::Active)
        return;
    if (!previewValid)
        return;
    commit();
}

void ChamferTool::onCommit()
{
    if (!targetCurve || targetId == 0 || !previewValid)
        return;

    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto command = std::make_unique<Tools::ApplyChamferCommand>(targetId, options, QStringLiteral("Round Corner"));
        stack->push(std::move(command));
        executed = true;
    }
    if (!executed) {
        Phase6::RoundCorner round(*geometry);
        round.filletCurve(*targetCurve, options);
    }
    resetState();
}

void ChamferTool::onCancel()
{
    resetState();
}

void ChamferTool::onStateChanged(State previous, State next)
{
    if (next == State::Idle && previous != State::Idle)
        resetState();
}

Tool::PreviewState ChamferTool::buildPreview() const
{
    PreviewState state;
    if (!previewValid)
        return state;
    PreviewPolyline polyline;
    polyline.points = previewLoop;
    polyline.closed = true;
    state.polylines.push_back(std::move(polyline));
    return state;
}

bool ChamferTool::rebuildPreview()
{
    previewLoop.clear();
    previewHardness.clear();
    previewValid = false;

    if (!targetCurve || !geometry)
        return false;

    Phase6::RoundCorner round(*geometry);
    std::unique_ptr<Curve> preview = round.createFilleted(*targetCurve, options);
    if (!preview)
        return false;
    previewLoop = preview->getBoundaryLoop();
    previewHardness = preview->getEdgeHardness();
    previewValid = previewLoop.size() >= 3;
    return previewValid;
}

void ChamferTool::resetState()
{
    previewLoop.clear();
    previewHardness.clear();
    previewValid = false;
    targetCurve = nullptr;
    targetId = 0;
}
