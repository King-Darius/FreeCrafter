#pragma once

#include <memory>
#include <vector>

#include "Tool.h"
#include "LineTool.h"
#include "SmartSelectTool.h"
#include "MoveTool.h"
#include "RotateTool.h"
#include "ScaleTool.h"
#include "ExtrudeTool.h"

#include "../Interaction/InferenceEngine.h"

struct ToolInferenceUpdateRequest {
    bool hasRay = false;
    Interaction::PickRay ray;
    float pixelRadius = 6.0f;
};

class ToolManager {
public:
    ToolManager(GeometryKernel* g, CameraController* c);
    Tool* getActiveTool() const { return active; }
    void activateTool(const char* name);
    void setViewportSize(int w, int h);

    void updateInference(const ToolInferenceUpdateRequest& request);
    void clearInference();
    const Interaction::InferenceResult& getCurrentInference() const { return currentInference; }
    void handleKeyPress(int key);
    void handleKeyRelease(int key);
    void updatePointerModifiers(const Tool::ModifierState& modifiers);

private:
    void propagateViewport();
    void pushInferenceToActive();
    void updateStickyLock();
    void applyAxisLock(const ToolInferenceUpdateRequest& request);
    void setAxisLock(const Vector3& direction);

    std::vector<std::unique_ptr<Tool>> tools;
    Tool* active = nullptr;
    int viewportWidth = 1;
    int viewportHeight = 1;
    GeometryKernel* geometry = nullptr;
    CameraController* camera = nullptr;
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
};

