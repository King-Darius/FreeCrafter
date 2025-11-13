#pragma once

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QUndoStack>

#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

#include "Core/CommandStack.h"
#include "GeometryKernel/GeometryObject.h"
#include "Scene/Document.h"

namespace TestSupport {

inline bool isOpenGLAvailable()
{
    static std::once_flag flag;
    static bool available = false;
    std::call_once(flag, []() {
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setRenderableType(QSurfaceFormat::OpenGL);
        QOpenGLContext context;
        context.setFormat(format);
        if (!context.create()) {
            available = false;
            return;
        }
        QOffscreenSurface surface;
        surface.setFormat(context.format());
        surface.create();
        if (!surface.isValid()) {
            available = false;
            return;
        }
        if (!context.makeCurrent(&surface)) {
            available = false;
            return;
        }
        context.doneCurrent();
        available = true;
    });
    return available;
}

inline bool ensureOpenGL(const char* testName)
{
    if (!isOpenGLAvailable()) {
        std::fprintf(stderr,
                     "[SKIP] %s: OpenGL context is unavailable on this platform.\n",
                     testName ? testName : "test");
        return false;
    }
    return true;
}

struct ToolCommandHarness {
    Scene::Document document;
    GeometryKernel& geometry;
    QUndoStack undoStack;
    Core::CommandStack commandStack;

    ToolCommandHarness()
        : document()
        , geometry(document.geometry())
        , undoStack()
        , commandStack(&undoStack)
    {
        Core::CommandContext context;
        context.document = &document;
        context.geometry = &geometry;
        context.geometryChanged = []() {};
        context.selectionChanged = [](const std::vector<Scene::Document::ObjectId>&) {};
        commandStack.setContext(context);
    }

    template <typename ToolT>
    void configureTool(ToolT& tool)
    {
        tool.setDocument(&document);
        tool.setGeometry(&geometry);
        tool.setCommandStack(&commandStack);
    }

    Scene::Document::ObjectId registerObject(GeometryObject* object, const std::string& name = std::string())
    {
        if (!object)
            return 0;
        return document.ensureObjectForGeometry(object, name);
    }
};

} // namespace TestSupport

