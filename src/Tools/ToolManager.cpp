#include "ToolManager.h"

#include <QtCore/Qt>

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

ToolManager::ToolManager(Scene::Document* doc, CameraController* c)
    : geometry(doc ? &doc->geometry() : nullptr)
    , camera(c)
    , document(doc)
{
    GeometryKernel* gPtr = geometry;
    tools.push_back(std::make_unique<SmartSelectTool>(gPtr, c));
    tools.push_back(std::make_unique<LineTool>(gPtr, c));
    tools.push_back(std::make_unique<ArcTool>(gPtr, c));
    tools.push_back(std::make_unique<CircleTool>(gPtr, c));
    tools.push_back(std::make_unique<PolygonTool>(gPtr, c));
    tools.push_back(std::make_unique<RotatedRectangleTool>(gPtr, c));
    tools.push_back(std::make_unique<FreehandTool>(gPtr, c));
    tools.push_back(std::make_unique<MoveTool>(gPtr, c));
    tools.push_back(std::make_unique<RotateTool>(gPtr, c));
    tools.push_back(std::make_unique<ScaleTool>(gPtr, c));
    tools.push_back(std::make_unique<ExtrudeTool>(gPtr, c));
    tools.push_back(std::make_unique<OffsetTool>(gPtr, c));
    tools.push_back(std::make_unique<FollowMeTool>(gPtr, c));
    tools.push_back(std::make_unique<PaintBucketTool>(gPtr, c));
    tools.push_back(std::make_unique<TextTool>(gPtr, c, document));
    tools.push_back(std::make_unique<DimensionTool>(gPtr, c, document));
    tools.push_back(std::make_unique<TapeMeasureTool>(gPtr, c, document));
    tools.push_back(std::make_unique<ProtractorTool>(gPtr, c, document));
    tools.push_back(std::make_unique<AxesTool>(gPtr, c, document));
    tools.push_back(std::make_unique<SectionTool>(geometry, c, document));
    tools.push_back(std::make_unique<OrbitTool>(gPtr, c));
    tools.push_back(std::make_unique<PanTool>(gPtr, c));
    tools.push_back(std::make_unique<ZoomTool>(gPtr, c));
    active = tools.empty() ? nullptr : tools.front().get();
    if (active && !active->isNavigationTool()) {
        lastModelingTool = active;
    }
    propagateViewport();
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
    return true;
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

