#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include "GeometryKernel/GeometryKernel.h"
#include "CameraController.h"

class ToolManager;
class QMouseEvent;
class QKeyEvent;

class GLViewport : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GLViewport(QWidget *parent = nullptr);

    GeometryKernel* getGeometry() { return &geometry; }
    CameraController* getCamera() { return &camera; }

    void setToolManager(ToolManager* manager);
    ToolManager* getToolManager() const { return toolManager; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    GeometryKernel geometry;
    CameraController camera;
    ToolManager* toolManager = nullptr;
};

