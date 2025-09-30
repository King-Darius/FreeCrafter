#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QPoint>
#include <QElapsedTimer>
#include <QVector3D>
#include <QString>
#include <QMatrix4x4>

#include "GeometryKernel/GeometryKernel.h"
#include "CameraController.h"
#include "Renderer.h"
#include "Navigation/ViewPresetManager.h"
#include "NavigationConfig.h"

#include <optional>

class ToolManager;
class NavigationPreferences;

class QPainter;

class GLViewport : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit GLViewport(QWidget* parent = nullptr);

    void setToolManager(ToolManager* manager);
    void setNavigationPreferences(NavigationPreferences* prefs);
    void setRenderStyle(Renderer::RenderStyle style);
    Renderer::RenderStyle getRenderStyle() const { return renderStyle; }
    void setShowHiddenGeometry(bool show);
    bool isHiddenGeometryVisible() const { return showHiddenGeometry; }
    ToolManager* getToolManager() const { return toolManager; }
    GeometryKernel* getGeometry() { return &geometry; }
    CameraController* getCamera() { return &camera; }
    CameraController::ProjectionMode projectionMode() const { return camera.getProjectionMode(); }
    void setProjectionMode(CameraController::ProjectionMode mode);
    void toggleProjectionMode();
    float fieldOfView() const { return camera.getFieldOfView(); }
    void setFieldOfView(float degrees);
    float orthoHeight() const { return camera.getOrthoHeight(); }
    void setOrthoHeight(float height);
    bool applyViewPreset(ViewPresetManager::StandardView view);
    bool applyViewPreset(const QString& id);
    QString currentViewPresetId() const { return activePresetId; }
    QString currentViewPresetLabel() const;
    void zoomInStep();
    void zoomOutStep();
    bool zoomExtents();
    bool zoomSelection();

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
    void keyReleaseEvent(QKeyEvent* e) override;
    void leaveEvent(QEvent* event) override;

private:
    void drawAxes();
    void drawGrid();
    void drawScene();
    void drawAxisGizmo(QPainter& painter, const QMatrix4x4& viewMatrix) const;
    QMatrix4x4 buildProjectionMatrix(float aspect) const;
    QMatrix4x4 buildViewMatrix() const;
    bool projectCursorToGround(const QPointF& pos, QVector3D& world) const;
    bool computePickRay(const QPoint& devicePos, QVector3D& origin, QVector3D& direction) const;
    bool projectWorldToScreen(const QVector3D& world, const QMatrix4x4& projection, const QMatrix4x4& view, QPointF& out) const;
    void drawInferenceOverlay(QPainter& painter, const QMatrix4x4& projection, const QMatrix4x4& view);
    bool computeBounds(bool selectedOnly, Vector3& outMin, Vector3& outMax) const;
    bool applyZoomToBounds(const Vector3& minBounds, const Vector3& maxBounds);

    GeometryKernel geometry;
    CameraController camera;
    ToolManager* toolManager = nullptr;
    NavigationPreferences* navigationPrefs = nullptr;
    NavigationConfig navigationConfig;

    QPoint lastMouse;
    QPoint lastDeviceMouse;
    bool hasDeviceMouse = false;
    Qt::MouseButton navigationButton = Qt::NoButton;
    std::optional<NavigationConfig::DragBinding> activeNavigationBinding;

    QTimer repaintTimer;
    QElapsedTimer frameTimer;
    double smoothedFps = 0.0;
    double smoothedFrameMs = 0.0;
    int lastDrawCalls = 0;
    mutable int currentDrawCalls = 0;
    Renderer renderer;
    Renderer::RenderStyle renderStyle = Renderer::RenderStyle::ShadedWithEdges;
    bool showHiddenGeometry = false;
    ViewPresetManager viewPresets;
    QString activePresetId = QStringLiteral("iso");
};
