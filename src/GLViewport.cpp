#include "GLViewport.h"

#include <QMouseEvent>

GLViewport::GLViewport(QWidget *parent) : QOpenGLWidget(parent) {}

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

