#include <cassert>
#include <string>

#include "CameraController.h"
#include "Scene/Document.h"
#include "Tools/ToolManager.h"

using ToolKind = Tool::MeasurementKind;

void testDefaultTool()
{
    Scene::Document document;
    CameraController camera;
    ToolManager manager(&document, &camera);

    assert(manager.getActiveTool() != nullptr);
    assert(std::string(manager.getActiveTool()->getName()) == "SmartSelectTool");
    assert(manager.getMeasurementKind() == ToolKind::None);
}

void testActivations()
{
    Scene::Document document;
    CameraController camera;
    ToolManager manager(&document, &camera);

    manager.activateTool("LineTool");
    assert(manager.getActiveTool() != nullptr);
    assert(std::string(manager.getActiveTool()->getName()) == "LineTool");
    assert(manager.getMeasurementKind() == ToolKind::Distance);

    manager.activateTool("MoveTool");
    assert(std::string(manager.getActiveTool()->getName()) == "MoveTool");
    assert(manager.getMeasurementKind() == ToolKind::Distance);

    // Unknown tool ids should leave the active tool untouched.
    manager.activateTool("DoesNotExist");
    assert(std::string(manager.getActiveTool()->getName()) == "MoveTool");
}

int main()
{
    testDefaultTool();
    testActivations();
    return 0;
}
