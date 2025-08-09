#include "GLViewport.h"

#include <QMouseEvent>
#include <QKeyEvent>

#include "Tools/ToolManager.h"

GLViewport::GLViewport(QWidget *parent) : QOpenGLWidget(parent) {}

void GLViewport::setToolManager(ToolManager* manager) {
    toolManager = manager;
}

void GLViewport::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
}

void GLViewport::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLViewport::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLViewport::mousePressEvent(QMouseEvent* event) {
    if (auto* tool = toolManager ? toolManager->getActiveTool() : nullptr) {
        auto pos = event->position().toPoint();
        tool->onMouseDown(pos.x(), pos.y());
    }
    QOpenGLWidget::mousePressEvent(event);
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    if (auto* tool = toolManager ? toolManager->getActiveTool() : nullptr) {
        auto pos = event->position().toPoint();
        tool->onMouseMove(pos.x(), pos.y());
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    if (auto* tool = toolManager ? toolManager->getActiveTool() : nullptr) {
        auto pos = event->position().toPoint();
        tool->onMouseUp(pos.x(), pos.y());
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GLViewport::keyPressEvent(QKeyEvent* event) {
    if (auto* tool = toolManager ? toolManager->getActiveTool() : nullptr) {
        tool->onKeyPress(static_cast<char>(event->key()));
    }
    QOpenGLWidget::keyPressEvent(event);
}

