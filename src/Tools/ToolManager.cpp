#include "ToolManager.h"

#include "ToolRegistry.h"
#include "ZoomTool.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <QtCore/QByteArray>
#include <QtCore/Qt>
#include <QtCore/QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

using Interaction::InferenceContext;
using Interaction::InferenceResult;
using Interaction::InferenceSnapType;

namespace {
constexpr float kLockEpsilon = 1e-5f;

Vector3 normalizeOrZero(const Vector3& value)
{
    float len = value.length();
    if (len <= kLockEpsilon) {
        return Vector3();
    }
    return value / len;
}
}

ToolManager::ToolManager(Scene::Document* doc, CameraController* c, Core::CommandStack* stack)
    : geometry(doc ? &doc->geometry() : nullptr)
    , camera(c)
    , document(doc)
    , commandStack(stack)
{
    const auto& registry = ToolRegistry::instance();
    ToolRegistry::ToolCreationContext context{ geometry, camera, document };
    tools.reserve(registry.allTools().size());
    for (const auto& descriptor : registry.allTools()) {
        if (!descriptor.factory)
            continue;
        auto tool = descriptor.factory(context);
        if (!tool)
            continue;
        Q_ASSERT(std::strcmp(tool->getName(), descriptor.idLiteral) == 0);
        tool->setDocument(document);
        tool->setCommandStack(commandStack);
        tools.push_back(std::move(tool));
    }
    active = tools.empty() ? nullptr : tools.front().get();
    if (active && !active->isNavigationTool()) {
        lastModelingTool = active;
    }
    propagateViewport();
    geometryRevision = geometry ? geometry->revision() : 0;
    pushInferenceToActive();
    if (active) {
        Tool::ModifierState mods{ shiftPressed, ctrlPressed, altPressed };
        active->setModifiers(mods);
    }
    setNavigationConfig(navigationConfig);
}

void ToolManager::activateTool(const char* name, bool temporary)
{
    Tool* found = nullptr;
    for (auto& tool : tools) {
        if (std::strcmp(tool->getName(), name) == 0) {
            found = tool.get();
            break;
        }
    }
    if (!found)
        return;

    if (temporary) {
        if (!temporaryTool)
            previousTool = active;
        temporaryTool = found;
        active = found;
    } else {
        active = found;
        temporaryTool = nullptr;
        previousTool = nullptr;
        if (!found->isNavigationTool())
            lastModelingTool = found;
    }

    pushInferenceToActive();
    if (active) {
        active->setViewportSize(viewportWidth, viewportHeight);
        Tool::ModifierState mods{ shiftPressed, ctrlPressed, altPressed };
        active->setModifiers(mods);
    }
}

void ToolManager::restorePreviousTool()
{
    if (!temporaryTool)
        return;

    Tool* target = previousTool;
    if (!target)
        target = lastModelingTool;
    if (!target && !tools.empty())
        target = tools.front().get();

    temporaryTool = nullptr;
    previousTool = nullptr;

    if (target)
        active = target;

    pushInferenceToActive();
    if (active) {
        active->setViewportSize(viewportWidth, viewportHeight);
        Tool::ModifierState mods{ shiftPressed, ctrlPressed, altPressed };
        active->setModifiers(mods);
    }
}

void ToolManager::setNavigationConfig(const NavigationConfig& config)
{
    navigationConfig = config;
    for (auto& tool : tools) {
        if (auto* zoom = dynamic_cast<ZoomTool*>(tool.get())) {
            zoom->setZoomToCursor(navigationConfig.zoomToCursor);
            zoom->setDragSensitivity(std::max(0.02f, navigationConfig.wheelStep * 0.08f));
        }
    }
}

void ToolManager::setViewportSize(int w, int h)
{
    viewportWidth = std::max(1, w);
    viewportHeight = std::max(1, h);
    propagateViewport();
}

void ToolManager::handlePointerDown(const Tool::PointerInput& input)
{
    if (!active)
        return;
    active->handleMouseDown(input);
    handleToolInteraction();
}

void ToolManager::handlePointerMove(const Tool::PointerInput& input)
{
    if (!active)
        return;
    active->handleMouseMove(input);
    handleToolInteraction();
}

void ToolManager::handlePointerUp(const Tool::PointerInput& input)
{
    if (!active)
        return;
    active->handleMouseUp(input);
    handleToolInteraction();
}

void ToolManager::commitActiveTool()
{
    if (!active)
        return;
    active->commit();
    handleToolInteraction();
}

void ToolManager::cancelActiveTool()
{
    if (!active)
        return;
    active->cancel();
    handleToolInteraction();
}

void ToolManager::updateInference(const ToolInferenceUpdateRequest& request)
{
    if (!geometry) {
        clearInference();
        return;
    }

    if (!request.hasRay) {
        currentInference = stickyActive ? stickyInference : InferenceResult{};
        pushInferenceToActive();
        return;
    }

    InferenceContext context;
    context.ray = request.ray;
    context.maxSnapDistance = 0.35f;
    context.pixelRadius = request.pixelRadius;
    if (camera) {
        float tx, ty, tz;
        camera->getTarget(tx, ty, tz);
        context.cameraTarget = Vector3(tx, ty, tz);
    }

    currentInference = inferenceEngine.query(*geometry, context);
    if (currentInference.isValid()) {
        lastSnapPoint = currentInference.position;
        lastSnapValid = true;
        if (!axisAnchorValid) {
            axisAnchor = currentInference.position;
            axisAnchorValid = true;
        }
    }

    updateStickyLock();
    if (axisLocked) {
        applyAxisLock(request);
    } else if (currentInference.isValid()) {
        axisAnchor = currentInference.position;
        axisAnchorValid = true;
    }

    pushInferenceToActive();
}

void ToolManager::clearInference()
{
    currentInference = stickyActive ? stickyInference : InferenceResult{};
    pushInferenceToActive();
}

void ToolManager::handleKeyPress(int key)
{
    if (key == Qt::Key_Shift) {
        shiftPressed = true;
        if (!stickyActive && currentInference.isValid()) {
            stickyInference = currentInference;
            stickyInference.locked = true;
            stickyActive = true;
        }
        pushInferenceToActive();
        if (active) {
            active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
        }
        return;
    }

    if (key == Qt::Key_Control || key == Qt::Key_Meta) {
        ctrlPressed = true;
        if (active) {
            active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
        }
    } else if (key == Qt::Key_Alt) {
        altPressed = true;
        if (active) {
            active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
        }
    }

    switch (key) {
    case Qt::Key_Left:
        setAxisLock(Vector3(-1.0f, 0.0f, 0.0f));
        break;
    case Qt::Key_Right:
        setAxisLock(Vector3(1.0f, 0.0f, 0.0f));
        break;
    case Qt::Key_Up:
        setAxisLock(Vector3(0.0f, 1.0f, 0.0f));
        break;
    case Qt::Key_Down:
        setAxisLock(Vector3(0.0f, -1.0f, 0.0f));
        break;
    case Qt::Key_PageUp:
        setAxisLock(Vector3(0.0f, 0.0f, 1.0f));
        break;
    case Qt::Key_PageDown:
        setAxisLock(Vector3(0.0f, 0.0f, -1.0f));
        break;
    default:
        if (active) {
            active->handleKeyPress(key);
        }
        break;
    }
    handleToolInteraction();
}

void ToolManager::handleKeyRelease(int key)
{
    if (key == Qt::Key_Shift) {
        shiftPressed = false;
        stickyActive = false;
        stickyInference = InferenceResult{};
        if (active) {
            pushInferenceToActive();
            active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
        }
        return;
    }

    if (key == Qt::Key_Control || key == Qt::Key_Meta) {
        ctrlPressed = false;
        if (active) {
            active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
        }
    } else if (key == Qt::Key_Alt) {
        altPressed = false;
        if (active) {
            active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
        }
    }

    switch (key) {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        axisLocked = false;
        pushInferenceToActive();
        break;
    default:
        if (active) {
            active->handleKeyRelease(key);
        }
        break;
    }
    handleToolInteraction();
}

void ToolManager::updatePointerModifiers(const Tool::ModifierState& modifiers)
{
    bool changed = false;
    if (shiftPressed != modifiers.shift) {
        shiftPressed = modifiers.shift;
        changed = true;
    }
    if (ctrlPressed != modifiers.ctrl) {
        ctrlPressed = modifiers.ctrl;
        changed = true;
    }
    if (altPressed != modifiers.alt) {
        altPressed = modifiers.alt;
        changed = true;
    }
    if (changed && active) {
        active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
    } else if (active) {
        active->setModifiers({ shiftPressed, ctrlPressed, altPressed });
    }
}

ToolCursorOverlayState ToolManager::cursorOverlayState() const
{
    ToolCursorOverlayState state;
    state.modifiers = { shiftPressed, ctrlPressed, altPressed };
    state.stickyLock = stickyActive;
    state.axisLocked = axisLocked;
    state.axisDirection = axisDirection;
    state.inference = stickyActive ? stickyInference : currentInference;
    if (active) {
        state.hasTool = true;
        state.toolName = QString::fromLatin1(active->getName());
        state.descriptor = active->cursorDescriptor();
    }
    return state;
}

Tool::MeasurementKind ToolManager::getMeasurementKind() const
{
    if (!active) {
        return Tool::MeasurementKind::None;
    }
    return active->getMeasurementKind();
}

bool ToolManager::applyMeasurementOverride(double value)
{
    if (!active) {
        return false;
    }
    Tool::OverrideResult result = active->applyMeasurementOverride(value);
    if (result == Tool::OverrideResult::Ignored) {
        return false;
    }
    if (result == Tool::OverrideResult::Commit) {
        active->commit();
    }
    handleToolInteraction();
    return true;
}

void ToolManager::setGeometryChangedCallback(std::function<void()> callback)
{
    geometryChangedCallback = std::move(callback);
}

void ToolManager::setCommandStack(Core::CommandStack* stack)
{
    commandStack = stack;
    for (auto& tool : tools) {
        tool->setCommandStack(commandStack);
    }
}

void ToolManager::notifyExternalGeometryChange()
{
    if (!geometry)
        return;
    geometryRevision = geometry->revision();
    if (document)
        document->synchronizeWithGeometry();
    if (geometryChangedCallback)
        geometryChangedCallback();
}

bool ToolManager::hasTool(const QString& name) const
{
    return hasTool(name.toUtf8().constData());
}

bool ToolManager::hasTool(const char* name) const
{
    if (!name)
        return false;
    for (const auto& tool : tools) {
        if (std::strcmp(tool->getName(), name) == 0)
            return true;
    }
    return false;
}

void ToolManager::propagateViewport()
{
    for (auto& tool : tools) {
        tool->setViewportSize(viewportWidth, viewportHeight);
    }
}

void ToolManager::pushInferenceToActive()
{
    if (active) {
        active->setInferenceResult(currentInference);
    }
}

void ToolManager::updateStickyLock()
{
    if (!shiftPressed) {
        stickyActive = false;
        stickyInference = InferenceResult{};
        return;
    }

    if (!stickyActive && currentInference.isValid()) {
        stickyInference = currentInference;
        stickyInference.locked = true;
        stickyActive = true;
    }

    if (stickyActive) {
        currentInference = stickyInference;
        currentInference.locked = true;
    }
}

void ToolManager::applyAxisLock(const ToolInferenceUpdateRequest& request)
{
    if (!axisLocked || !request.hasRay) {
        return;
    }

    Vector3 direction = normalizeOrZero(axisDirection);
    if (direction.lengthSquared() <= kLockEpsilon) {
        axisLocked = false;
        return;
    }

    if (!axisAnchorValid) {
        if (currentInference.isValid()) {
            axisAnchor = currentInference.position;
            axisAnchorValid = true;
        } else if (stickyActive) {
            axisAnchor = stickyInference.position;
            axisAnchorValid = true;
        } else if (lastSnapValid) {
            axisAnchor = lastSnapPoint;
            axisAnchorValid = true;
        } else if (camera) {
            float tx, ty, tz;
            camera->getTarget(tx, ty, tz);
            axisAnchor = Vector3(tx, ty, tz);
            axisAnchorValid = true;
        } else {
            axisAnchor = Vector3();
            axisAnchorValid = true;
        }
    }

    Vector3 rayDir = normalizeOrZero(request.ray.direction);
    if (rayDir.lengthSquared() <= kLockEpsilon) {
        return;
    }
    Vector3 rayOrigin = request.ray.origin;

    Vector3 w0 = rayOrigin - axisAnchor;
    float a = rayDir.dot(rayDir);
    float b = rayDir.dot(direction);
    float c = direction.dot(direction);
    float d = rayDir.dot(w0);
    float e = direction.dot(w0);
    float denom = a * c - b * b;
    float s = 0.0f;
    float t = 0.0f;
    if (std::fabs(denom) > kLockEpsilon) {
        s = (b * e - c * d) / denom;
        t = (a * e - b * d) / denom;
    } else {
        t = -e / (c > kLockEpsilon ? c : 1.0f);
    }
    if (!stickyActive && t < 0.0f) {
        t = 0.0f;
    }
    Vector3 axisPoint = axisAnchor + direction * t;
    Vector3 rayPoint = rayOrigin + rayDir * s;
    Vector3 delta = axisPoint - rayPoint;

    InferenceResult axisResult;
    axisResult.type = InferenceSnapType::Axis;
    axisResult.position = axisPoint;
    axisResult.direction = direction;
    axisResult.reference = axisAnchor;
    axisResult.distance = delta.lengthSquared();
    axisResult.locked = stickyActive || axisLocked;
    currentInference = axisResult;
}

void ToolManager::setAxisLock(const Vector3& direction)
{
    Vector3 norm = normalizeOrZero(direction);
    if (norm.lengthSquared() <= kLockEpsilon) {
        axisLocked = false;
        return;
    }

    if (axisLocked) {
        Vector3 diff = axisDirection - norm;
        if (diff.lengthSquared() < 1e-4f) {
            axisLocked = false;
            return;
        }
    }

    axisLocked = true;
    axisDirection = norm;
    if (currentInference.isValid()) {
        axisAnchor = currentInference.position;
        axisAnchorValid = true;
    } else if (stickyActive) {
        axisAnchor = stickyInference.position;
        axisAnchorValid = true;
    } else if (lastSnapValid) {
        axisAnchor = lastSnapPoint;
        axisAnchorValid = true;
    } else if (camera) {
        float tx, ty, tz;
        camera->getTarget(tx, ty, tz);
        axisAnchor = Vector3(tx, ty, tz);
        axisAnchorValid = true;
    } else {
        axisAnchor = Vector3();
        axisAnchorValid = true;
    }
}

void ToolManager::handleToolInteraction()
{
    if (!geometry)
        return;
    std::uint64_t current = geometry->revision();
    if (current == geometryRevision)
        return;
    geometryRevision = current;
    if (document)
        document->synchronizeWithGeometry();
    if (geometryChangedCallback)
        geometryChangedCallback();
}

