#include "Tool.h"

#include <algorithm>
#include <QString>

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

Tool::CursorDescriptor Tool::cursorDescriptor() const
{
    CursorDescriptor descriptor;
    const QString name = QString::fromLatin1(getName());

    if (name == QLatin1String("PanTool")) {
        descriptor.mode = CursorDescriptor::Mode::Navigate;
        descriptor.preferSystemCursor = true;
        return descriptor;
    }
    if (name == QLatin1String("OrbitTool")) {
        descriptor.mode = CursorDescriptor::Mode::Navigate;
        descriptor.preferSystemCursor = true;
        return descriptor;
    }
    if (name == QLatin1String("ZoomTool")) {
        descriptor.mode = CursorDescriptor::Mode::Navigate;
        descriptor.preferSystemCursor = true;
        return descriptor;
    }
    if (name == QLatin1String("Text")) {
        descriptor.mode = CursorDescriptor::Mode::Annotate;
        descriptor.preferSystemCursor = true;
        return descriptor;
    }

    const auto configureDrawingCursor = [&descriptor]() {
        descriptor.mode = CursorDescriptor::Mode::Draw;
        descriptor.showCrosshair = true;
        descriptor.showPickCircle = true;
        descriptor.modifierHint = QStringLiteral("Shift: Stick inference  •  X/Y/Z: Axis lock");
    };

    if (name == QLatin1String("SmartSelectTool")) {
        descriptor.mode = CursorDescriptor::Mode::Pointer;
        descriptor.showPickCircle = true;
        descriptor.pickRadius = 5.5f;
        descriptor.modifierHint = QStringLiteral("Shift: Add  •  Ctrl: Toggle");
        return descriptor;
    }
    if (name == QLatin1String("LineTool") || name == QLatin1String("Rectangle")
        || name == QLatin1String("Arc") || name == QLatin1String("CenterArc")
        || name == QLatin1String("TangentArc") || name == QLatin1String("Circle")
        || name == QLatin1String("Polygon") || name == QLatin1String("RotatedRectangle")
        || name == QLatin1String("Freehand") || name == QLatin1String("Bezier")
        || name == QLatin1String("Offset") || name == QLatin1String("PushPull")
        || name == QLatin1String("FollowMe") || name == QLatin1String("ExtrudeTool")
        || name == QLatin1String("ChamferTool") || name == QLatin1String("LoftTool")
        || name == QLatin1String("SectionTool")) {
        configureDrawingCursor();
        return descriptor;
    }
    if (name == QLatin1String("Dimension") || name == QLatin1String("TapeMeasure")
        || name == QLatin1String("Protractor") || name == QLatin1String("Axes")) {
        configureDrawingCursor();
        descriptor.mode = CursorDescriptor::Mode::Annotate;
        return descriptor;
    }
    if (name == QLatin1String("PaintBucket")) {
        descriptor.mode = CursorDescriptor::Mode::Annotate;
        descriptor.showPickCircle = true;
        descriptor.pickRadius = 7.0f;
        descriptor.modifierHint = QStringLiteral("Alt: Sample material");
        return descriptor;
    }
    if (name == QLatin1String("MoveTool")) {
        descriptor.mode = CursorDescriptor::Mode::Move;
        descriptor.showCrosshair = true;
        descriptor.showPickCircle = true;
        descriptor.pickRadius = 6.5f;
        descriptor.modifierHint = QStringLiteral("Ctrl: Copy  •  Shift: Lock axis");
        return descriptor;
    }
    if (name == QLatin1String("RotateTool")) {
        descriptor.mode = CursorDescriptor::Mode::Rotate;
        descriptor.showCrosshair = true;
        descriptor.showPickCircle = true;
        descriptor.pickRadius = 7.5f;
        descriptor.modifierHint = QStringLiteral("Shift: 15° snap  •  Ctrl: Copy");
        return descriptor;
    }
    if (name == QLatin1String("ScaleTool")) {
        descriptor.mode = CursorDescriptor::Mode::Move;
        descriptor.showCrosshair = true;
        descriptor.showPickCircle = true;
        descriptor.pickRadius = 6.0f;
        descriptor.modifierHint = QStringLiteral("Shift: Uniform  •  Ctrl: About center");
        return descriptor;
    }

    return descriptor;
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

