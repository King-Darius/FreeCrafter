#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

#include "Tool.h"

#include "../Interaction/InferenceEngine.h"
#include "../NavigationConfig.h"

#include <QString>

namespace Core {
class CommandStack;
}

namespace Scene {
class Document;
}

struct ToolInferenceUpdateRequest {
    bool hasRay = false;
    Interaction::PickRay ray;
    float pixelRadius = 6.0f;
};

struct ToolCursorOverlayState {
    bool hasTool = false;
    QString toolName;
    Tool::CursorDescriptor descriptor;
    Interaction::InferenceResult inference;
    bool stickyLock = false;
    bool axisLocked = false;
    Vector3 axisDirection;
    Tool::ModifierState modifiers;
};

class ToolManager {
public:
    ToolManager(Scene::Document* document, CameraController* c, Core::CommandStack* stack = nullptr);
    Tool* getActiveTool() const { return active; }
    void activateTool(const char* name, bool temporary = false);
    void restorePreviousTool();
    void setNavigationConfig(const NavigationConfig& config);
    void setViewportSize(int w, int h);
    void setCamera(CameraController* cameraController);
    void setDocument(Scene::Document* document);

    void handlePointerDown(const Tool::PointerInput& input);
    void handlePointerMove(const Tool::PointerInput& input);
    void handlePointerUp(const Tool::PointerInput& input);
    void commitActiveTool();
    void cancelActiveTool();

    void updateInference(const ToolInferenceUpdateRequest& request);
    void clearInference();
    const Interaction::InferenceResult& getCurrentInference() const { return currentInference; }
    void handleKeyPress(int key);
    void handleKeyRelease(int key);
    void updatePointerModifiers(const Tool::ModifierState& modifiers);
    ToolCursorOverlayState cursorOverlayState() const;

    Tool::MeasurementKind getMeasurementKind() const;
    bool applyMeasurementOverride(double value);

    void setGeometryChangedCallback(std::function<void()> callback);
    void setCommandStack(Core::CommandStack* stack);
    void notifyExternalGeometryChange();

    bool hasTool(const QString& name) const;
    bool hasTool(const char* name) const;

private:
    void propagateViewport();
    void pushInferenceToActive();
    void updateStickyLock();
    void applyAxisLock(const ToolInferenceUpdateRequest& request);
    void setAxisLock(const Vector3& direction);
    void handleToolInteraction();

    std::vector<std::unique_ptr<Tool>> tools;
    Tool* active = nullptr;
    Tool* lastModelingTool = nullptr;
    Tool* temporaryTool = nullptr;
    Tool* previousTool = nullptr;
    int viewportWidth = 1;
    int viewportHeight = 1;
    GeometryKernel* geometry = nullptr;
    CameraController* camera = nullptr;
    Scene::Document* document = nullptr;
    Core::CommandStack* commandStack = nullptr;
    Interaction::InferenceEngine inferenceEngine;
    Interaction::InferenceResult currentInference;
    Interaction::InferenceResult stickyInference;
    bool shiftPressed = false;
    bool ctrlPressed = false;
    bool altPressed = false;
    bool stickyActive = false;
    bool axisLocked = false;
    Vector3 axisDirection;
    Vector3 axisAnchor;
    bool axisAnchorValid = false;
    Vector3 lastSnapPoint;
    bool lastSnapValid = false;
    NavigationConfig navigationConfig;
    std::function<void()> geometryChangedCallback;
    std::uint64_t geometryRevision = 0;
};

