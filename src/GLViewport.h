#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QPoint>
#include <QElapsedTimer>
#include <QVector3D>

#include "GeometryKernel/GeometryKernel.h"
#include "CameraController.h"

class ToolManager;

class GLViewport : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit GLViewport(QWidget* parent = nullptr);

    void setToolManager(ToolManager* manager);
    ToolManager* getToolManager() const { return toolManager; }
    GeometryKernel* getGeometry() { return &geometry; }
    CameraController* getCamera() { return &camera; }

signals:
    void cursorPositionChanged(double x, double y, double z);
    void frameStatsUpdated(double fps, double frameMs, int drawCalls);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void leaveEvent(QEvent* event) override;

private:
    void drawAxes();
    void drawGrid();
    void drawScene();
    bool projectCursorToGround(const QPointF& pos, QVector3D& world) const;

    GeometryKernel geometry;
    CameraController camera;
    ToolManager* toolManager = nullptr;

    QPoint lastMouse;
    bool rotating = false;
    bool panning = false;

    QTimer repaintTimer;
    QElapsedTimer frameTimer;
    double smoothedFps = 0.0;
    double smoothedFrameMs = 0.0;
    int lastDrawCalls = 0;
    mutable int currentDrawCalls = 0;
};
