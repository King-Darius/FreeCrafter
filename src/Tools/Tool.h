#pragma once
#include "../GeometryKernel/GeometryKernel.h"
#include "../CameraController.h"
#include "../Interaction/InferenceEngine.h"

#include <algorithm>

class Tool {
public:
    Tool(GeometryKernel* g, CameraController* c) : geometry(g), camera(c) {}
    virtual ~Tool() = default;
    virtual void onMouseDown(int,int) {}
    virtual void onMouseMove(int,int) {}
    virtual void onMouseUp(int,int) {}
    virtual void onKeyPress(int) {}
    virtual void onKeyRelease(int) {}
    virtual void onInferenceResultChanged(const Interaction::InferenceResult&) {}
    virtual const char* getName() const = 0;

    void setViewportSize(int w, int h)
    {
        viewportWidth = std::max(1, w);
        viewportHeight = std::max(1, h);
    }

    const Interaction::InferenceResult& getInferenceResult() const { return currentInference; }
    void setInferenceResult(const Interaction::InferenceResult& result)
    {
        currentInference = result;
        onInferenceResultChanged(currentInference);
    }

protected:
    int viewportWidth = 1;
    int viewportHeight = 1;
    GeometryKernel* geometry; CameraController* camera;

private:
    Interaction::InferenceResult currentInference;
};
