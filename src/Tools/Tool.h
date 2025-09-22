#pragma once
#include "../GeometryKernel/GeometryKernel.h"
#include "../CameraController.h"

#include <string>

class Tool {
public:
    Tool(GeometryKernel* g, CameraController* c) : geometry(g), camera(c) {}
    virtual ~Tool() = default;
    virtual void onMouseDown(int,int) {}
    virtual void onMouseMove(int,int) {}
    virtual void onMouseUp(int,int) {}
    virtual void onKeyPress(char) {}
    virtual const char* getName() const = 0;
    virtual std::string getHint() const { return std::string(); }
    virtual std::string getMeasurementPrompt() const { return std::string(); }
    virtual bool acceptsNumericInput() const { return false; }
    virtual bool applyNumericInput(double) { return false; }
protected:
    GeometryKernel* geometry; CameraController* camera;
};
