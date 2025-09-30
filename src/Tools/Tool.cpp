#include "Tool.h"

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

