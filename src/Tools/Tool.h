#pragma once
#include "../GeometryKernel/GeometryKernel.h"
#include "../CameraController.h"

class Tool {
public:
    Tool(GeometryKernel* g, CameraController* c) : geometry(g), camera(c) {}
    virtual ~Tool() = default;
    virtual void onMouseDown(int,int) {}
    virtual void onMouseMove(int,int) {}
    virtual void onMouseUp(int,int) {}
    virtual void onKeyPress(char) {}
    virtual const char* getName() const = 0;
protected:
    GeometryKernel* geometry; CameraController* camera;
};
