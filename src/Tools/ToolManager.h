#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

#include "Tool.h"
#include "LineTool.h"
#include "SmartSelectTool.h"
#include "MoveTool.h"
#include "RotateTool.h"
#include "ScaleTool.h"
#include "ExtrudeTool.h"
#include "DrawingTools.h"
#include "ModificationTools.h"
#include "SectionTool.h"
#include "OrbitTool.h"
#include "PanTool.h"
#include "ZoomTool.h"

#include "../Interaction/InferenceEngine.h"
#include "../NavigationConfig.h"

namespace Core {
class CommandStack;
}

struct ToolInferenceUpdateRequest {
    bool hasRay = false;
    Interaction::PickRay ray;
    float pixelRadius = 6.0f;
};

class ToolManager {
public:
    ToolManager(Scene::Document* document, CameraController* c, Core::CommandStack* stack = nullptr);
    Tool* getActiveTool() const { return active; }
    void activateTool(const char* name, bool temporary = false);
    void restorePreviousTool();
    void setNavigationConfig(const NavigationConfig& config);
    void setViewportSize(int w, int h);

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

    Tool::MeasurementKind getMeasurementKind() const;
    bool applyMeasurementOverride(double value);

    void setGeometryChangedCallback(std::function<void()> callback);
    void setCommandStack(Core::CommandStack* stack);
    void notifyExternalGeometryChange();

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

