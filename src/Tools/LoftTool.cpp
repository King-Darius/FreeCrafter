#include "LoftTool.h"

#include "ToolCommands.h"
#include "../Core/CommandStack.h"

#include "../GeometryKernel/GeometryKernel.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr int kMinSections = 2;
}

LoftTool::LoftTool(GeometryKernel* g, CameraController* c)
    : Tool(g, c)
{
    options.sections = 8;
    options.closeRails = false;
    options.smoothNormals = true;
    options.twistDegrees = 0.0f;
    options.smoothingPasses = 1;
    options.symmetricPairing = false;
}

Tool::OverrideResult LoftTool::applyMeasurementOverride(double value)
{
    if (!std::isfinite(value))
        return Tool::OverrideResult::Ignored;
    int sections = static_cast<int>(std::round(value));
    if (sections < kMinSections)
        sections = kMinSections;
    if (sections == options.sections)
        return rebuildPreview() ? Tool::OverrideResult::PreviewUpdated : Tool::OverrideResult::Ignored;
    options.sections = sections;
    if (rebuildPreview())
        return Tool::OverrideResult::PreviewUpdated;
    return Tool::OverrideResult::Ignored;
}

void LoftTool::setLoftOptions(const Phase6::LoftOptions& opts)
{
    options = opts;
    options.sections = std::max(kMinSections, options.sections);
    options.smoothingPasses = std::max(0, options.smoothingPasses);
    rebuildPreview();
}

bool LoftTool::collectSelection()
{
    startCurve = nullptr;
    endCurve = nullptr;
    startId = 0;
    endId = 0;

    if (!geometry)
        return false;

    for (const auto& object : geometry->getObjects()) {
        if (!object || !object->isSelected())
            continue;
        if (object->getType() != ObjectType::Curve)
            continue;
        if (!startCurve) {
            startCurve = static_cast<Curve*>(object.get());
            if (Scene::Document* doc = getDocument())
                startId = doc->objectIdForGeometry(object.get());
        } else if (!endCurve) {
            endCurve = static_cast<Curve*>(object.get());
            if (Scene::Document* doc = getDocument())
                endId = doc->objectIdForGeometry(object.get());
            break;
        }
    }
    return startCurve && endCurve;
}

void LoftTool::onPointerDown(const PointerInput&)
{
    if (!collectSelection())
        return;
    if (!rebuildPreview()) {
        resetState();
        return;
    }
    setState(State::Active);
}

void LoftTool::onPointerUp(const PointerInput&)
{
    if (getState() != State::Active)
        return;
    if (!previewValid)
        return;
    commit();
}

void LoftTool::onCommit()
{
    if (!geometry || startId == 0 || endId == 0 || !previewValid)
        return;

    bool executed = false;
    if (auto* stack = getCommandStack()) {
        auto command = std::make_unique<Tools::CreateLoftCommand>(startId, endId, options, QStringLiteral("Loft"));
        stack->push(std::move(command));
        executed = true;
    }
    if (!executed) {
        Phase6::CurveIt op(*geometry);
        if (startCurve && endCurve)
            op.loft(*startCurve, *endCurve, options);
    }
    resetState();
}

void LoftTool::onCancel()
{
    resetState();
}

void LoftTool::onStateChanged(State previous, State next)
{
    if (next == State::Idle && previous != State::Idle)
        resetState();
}

Tool::PreviewState LoftTool::buildPreview() const
{
    PreviewState state;
    if (!previewValid || !previewSolid)
        return state;
    PreviewGhost ghost;
    ghost.object = previewSolid;
    state.ghosts.push_back(ghost);
    return state;
}

bool LoftTool::rebuildPreview()
{
    previewKernel.clear();
    previewSolid = nullptr;
    previewValid = false;

    if (!startCurve || !endCurve)
        return false;

    const auto& startLoop = startCurve->getBoundaryLoop();
    const auto& endLoop = endCurve->getBoundaryLoop();
    if (startLoop.size() < 3 || endLoop.size() < 3)
        return false;

    Curve* previewStart = static_cast<Curve*>(previewKernel.addCurve(startLoop));
    Curve* previewEnd = static_cast<Curve*>(previewKernel.addCurve(endLoop));
    if (!previewStart || !previewEnd)
        return false;

    Phase6::CurveIt previewOp(previewKernel);
    Solid* generated = previewOp.loft(*previewStart, *previewEnd, options);
    if (!generated)
        return false;
    previewSolid = generated;
    previewValid = true;
    return true;
}

void LoftTool::resetState()
{
    previewKernel.clear();
    previewSolid = nullptr;
    previewValid = false;
    startCurve = nullptr;
    endCurve = nullptr;
    startId = 0;
    endId = 0;
}
