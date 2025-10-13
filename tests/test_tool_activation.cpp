#include <cassert>
#include <string>

#include "CameraController.h"
#include "Scene/Document.h"
#include "Tools/ToolManager.h"
#include "Tools/ToolRegistry.h"

using ToolKind = Tool::MeasurementKind;

void testDefaultTool()
{
    Scene::Document document;
    CameraController camera;
    ToolManager manager(&document, &camera);

    assert(manager.getActiveTool() != nullptr);
    const auto& registry = ToolRegistry::instance();
    const auto& selectDescriptor = registry.descriptor(ToolRegistry::ToolId::SmartSelect);
    assert(std::string(manager.getActiveTool()->getName()) == selectDescriptor.idLiteral);
    assert(manager.getMeasurementKind() == ToolKind::None);
}

void testActivations()
{
    Scene::Document document;
    CameraController camera;
    ToolManager manager(&document, &camera);

    const auto& registry = ToolRegistry::instance();
    const auto& lineDescriptor = registry.descriptor(ToolRegistry::ToolId::Line);
    manager.activateTool(lineDescriptor.idLiteral);
    assert(manager.getActiveTool() != nullptr);
    assert(std::string(manager.getActiveTool()->getName()) == lineDescriptor.idLiteral);
    assert(manager.getMeasurementKind() == ToolKind::Distance);

    const auto& moveDescriptor = registry.descriptor(ToolRegistry::ToolId::Move);
    manager.activateTool(moveDescriptor.idLiteral);
    assert(std::string(manager.getActiveTool()->getName()) == moveDescriptor.idLiteral);
    assert(manager.getMeasurementKind() == ToolKind::Distance);

    // Unknown tool ids should leave the active tool untouched.
    manager.activateTool("DoesNotExist");
    assert(std::string(manager.getActiveTool()->getName()) == moveDescriptor.idLiteral);
}

int main()
{
    testDefaultTool();
    testActivations();
    return 0;
}
