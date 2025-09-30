#include "Tool.h"

#include <algorithm>

void Tool::handleMouseDown(const PointerInput& input)
{
    updateModifiers(input.modifiers);
    if (state == State::Idle) {
        setState(State::Armed);
    }
    onPointerDown(input);
}

void Tool::handleMouseMove(const PointerInput& input)
{
    updateModifiers(input.modifiers);
    if (state == State::Idle) {
        onPointerHover(input);
    } else {
        onPointerMove(input);
    }
}

void Tool::handleMouseUp(const PointerInput& input)
{
    updateModifiers(input.modifiers);
    onPointerUp(input);
    if (state == State::Armed) {
        setState(State::Idle);
    }
}

void Tool::handleHover(const PointerInput& input)
{
    updateModifiers(input.modifiers);
    onPointerHover(input);
}

void Tool::handleKeyPress(int key)
{
    onKeyDown(key);
}

void Tool::handleKeyRelease(int key)
{
    onKeyUp(key);
}

void Tool::commit()
{
    onCommit();
    if (state != State::Idle) {
        setState(State::Idle);
    }
}

void Tool::cancel()
{
    onCancel();
    if (state != State::Idle) {
        setState(State::Idle);
    }
}

void Tool::setModifiers(const ModifierState& newModifiers)
{
    updateModifiers(newModifiers);
}

void Tool::setState(State newState)
{
    if (newState == state) {
        return;
    }
    State previous = state;
    state = newState;
    onStateChanged(previous, state);
}

void Tool::updateModifiers(const ModifierState& nextModifiers)
{
    if (modifiers != nextModifiers) {
        modifiers = nextModifiers;
        onModifiersChanged(modifiers);
    }
}

void PointerDragTool::onPointerDown(const PointerInput& input)
{
    dragState.begin(input);
    setState(State::Active);
    onDragStart(input);
}

void PointerDragTool::onPointerMove(const PointerInput& input)
{
    float dx = 0.0f;
    float dy = 0.0f;
    if (!dragState.update(input, dx, dy)) {
        return;
    }
    onDragUpdate(input, dx, dy);
}

void PointerDragTool::onPointerUp(const PointerInput& input)
{
    if (dragState.finish(input)) {
        onDragEnd(input);
    }
    setState(State::Idle);
}

void PointerDragTool::onCancel()
{
    if (dragState.cancel()) {
        onDragCanceled();
    }
    setState(State::Idle);
}

void PointerDragTool::DragState::begin(const PointerInput& input)
{
    dragging = true;
    lastX = input.x;
    lastY = input.y;
    pixelScale = std::max(input.devicePixelRatio, 1.0f);
}

bool PointerDragTool::DragState::update(const PointerInput& input, float& dx, float& dy)
{
    if (!dragging) {
        lastX = input.x;
        lastY = input.y;
        return false;
    }

    float scale = std::max(pixelScale, 1.0f);
    dx = (input.x - lastX) / scale;
    dy = (input.y - lastY) / scale;
    lastX = input.x;
    lastY = input.y;
    return true;
}

bool PointerDragTool::DragState::finish(const PointerInput& input)
{
    lastX = input.x;
    lastY = input.y;
    bool wasDragging = dragging;
    dragging = false;
    return wasDragging;
}

bool PointerDragTool::DragState::cancel()
{
    bool wasDragging = dragging;
    dragging = false;
    return wasDragging;
}

