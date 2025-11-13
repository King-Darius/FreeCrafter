#include <cassert>
#include <string>

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QString>

#include "CameraController.h"
#include "GLViewport.h"
#include "MainWindow.h"
#include "Scene/Document.h"
#include "Tools/ToolManager.h"
#include "Tools/ToolRegistry.h"
#include "test_support.h"

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

void testUiBindings()
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    int argc = 1;
    char arg0[] = "tool_activation";
    char* argv[] = { arg0, nullptr };
    QApplication app(argc, argv);

    if (!TestSupport::ensureOpenGL("tool_activation_ui"))
        return;

    MainWindow window;
    GLViewport* viewport = window.findChild<GLViewport*>();
    assert(viewport != nullptr);

    ToolManager* manager = viewport->getToolManager();
    assert(manager != nullptr);

    const auto& registry = ToolRegistry::instance();
    for (const auto& descriptor : registry.allTools()) {
        assert(manager->hasTool(descriptor.id));
        QAction* action = window.findChild<QAction*>(descriptor.id);
        assert(action != nullptr);
        action->trigger();
        assert(manager->getActiveTool() != nullptr);
        assert(QString::fromUtf8(manager->getActiveTool()->getName()) == descriptor.id);
    }
}

int main()
{
    testDefaultTool();
    testActivations();
    testUiBindings();
    return 0;
}
