#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QTimer>
#include <QPoint>
#include <QElapsedTimer>
#include <QVector3D>
#include <QString>
#include <QMatrix4x4>
#include <QColor>
#include <QSize>
#include <QVariantAnimation>
#include <QEasingCurve>

#include <utility>
#include <cstdint>

#include "Scene/Document.h"
#include "GeometryKernel/GeometryKernel.h"
#include "CameraController.h"
#include "Renderer.h"
#include "Tools/Tool.h"
#include "PalettePreferences.h"
#include "SunSettings.h"
#include "Navigation/ViewPresetManager.h"
#include "NavigationConfig.h"

#include <optional>

class ToolManager;
class Tool;
class NavigationPreferences;
class QShowEvent;
class QHideEvent;

class QPainter;

class GLViewport : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit GLViewport(QWidget* parent = nullptr);

    void setToolManager(ToolManager* manager);
    void setNavigationPreferences(NavigationPreferences* prefs);
    void setPalettePreferences(PalettePreferences* prefs);
    void setRenderStyle(Renderer::RenderStyle style);
    Renderer::RenderStyle getRenderStyle() const { return renderStyle; }
    void setShowHiddenGeometry(bool show);
    bool isHiddenGeometryVisible() const { return showHiddenGeometry; }
    void setShowGrid(bool show);
    bool showGrid() const { return gridVisible; }
    void setFrameStatsVisible(bool visible);
    bool frameStatsVisible() const { return frameStatsHudVisible; }
    void setSunSettings(const SunSettings& settings);
    void setBackgroundPalette(const QColor& sky, const QColor& ground, const QColor& horizonLine);
    QColor skyBackgroundColor() const { return skyColor; }
    QColor groundBackgroundColor() const { return groundColor; }
    QColor horizonBackgroundColor() const { return horizonLineColor; }
    const SunSettings& sunSettings() const { return environmentSettings; }
    ToolManager* getToolManager() const { return toolManager; }
    GeometryKernel* getGeometry() { return &document.geometry(); }
    Scene::Document* getDocument() { return &document; }
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
    void setAutoFrameOnGeometryChange(bool enabled);
    bool autoFrameOnGeometryChangeEnabled() const { return autoFrameOnGeometryChange; }
    void requestAutoFrameOnGeometryChange();
    bool frameSceneToGeometry();
    void resetCameraToHome(bool triggerRedraw = true);
    std::pair<float, float> depthRangeForAspect(float aspect) const;
    bool computePickRay(const QPoint& devicePos, QVector3D& origin, QVector3D& direction) const;

    struct CursorOverlaySnapshot {
        bool hasTool = false;
        QString toolName;
        Tool::CursorDescriptor descriptor;
        QString inferenceLabel;
        QString badgeLabel;
        QString modifierHint;
        bool inferenceLocked = false;
        bool axisLocked = false;
        bool stickyLock = false;
        bool showOverlay = false;
    };

    CursorOverlaySnapshot queryCursorOverlaySnapshot();
    CursorOverlaySnapshot lastCursorOverlay() const { return cursorOverlaySnapshot; }

    void setAutoFocusSelection(bool enable);
    bool autoFocusSelectionEnabled() const { return autoFocusSelection; }
    bool focusSelection(bool animate = true);
    void notifySelectionChanged();

signals:
    void cursorPositionChanged(double x, double y, double z);
    void frameStatsUpdated(double fps, double frameMs, int drawCalls);
    void viewportResized(const QSize& size);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

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
    void initializeHorizonBand();
    void drawHorizonBand();
    void drawSceneGeometry();
    void drawSceneOverlays();
    QMatrix4x4 buildProjectionMatrix(float aspect) const;
    QMatrix4x4 buildViewMatrix() const;
    bool projectCursorToGround(const QPointF& pos, QVector3D& world) const;
    bool projectWorldToScreen(const QVector3D& world, const QMatrix4x4& projection, const QMatrix4x4& view, QPointF& out) const;
    void drawInferenceOverlay(QPainter& painter, const QMatrix4x4& projection, const QMatrix4x4& view);
    void drawCursorOverlay(QPainter& painter);
    CursorOverlaySnapshot buildCursorOverlaySnapshot() const;
    bool computeBounds(bool selectedOnly, bool includeHidden, Vector3& outMin, Vector3& outMax) const;
    bool frameSceneToGeometryInternal(bool includeHidden, bool triggerRedraw, bool animate = true);
    struct CameraKeyframe {
        QVector3D target;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float distance = 1.0f;
        CameraController::ProjectionMode projection = CameraController::ProjectionMode::Perspective;
        float fieldOfView = 60.0f;
        float orthoHeight = 1.0f;
    };
    CameraKeyframe captureCameraState() const;
    static CameraKeyframe cameraStateFromController(const CameraController& camera);
    void applyCameraState(const CameraKeyframe& state);
    static CameraKeyframe interpolateCameraState(const CameraKeyframe& from, const CameraKeyframe& to, float t);
    bool cameraStatesApproximatelyEqual(const CameraKeyframe& a, const CameraKeyframe& b) const;
    std::optional<CameraKeyframe> cameraStateForBounds(const Vector3& minBounds, const Vector3& maxBounds) const;
    bool applyCameraStateForBounds(const Vector3& minBounds, const Vector3& maxBounds, bool triggerRedraw, bool animate);
    void startCameraAnimation(const CameraKeyframe& targetState, int durationMs = 240);
    void stopCameraAnimation(bool applyEndState);
    void applyAnimatedCameraProgress(float t);
    void cancelAnimationsForImmediateInput();
    void refreshCursorShape();

    Scene::Document document;
    CameraController camera;
    ToolManager* toolManager = nullptr;
    NavigationPreferences* navigationPrefs = nullptr;
    NavigationConfig navigationConfig;
    PalettePreferences* palettePrefs = nullptr;
    PalettePreferences::ColorSet paletteColors;

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
    bool gridVisible = true;
    bool frameStatsHudVisible = false;
    SunSettings environmentSettings;
    ViewPresetManager viewPresets;
    QString activePresetId = QStringLiteral("iso");
    Qt::CursorShape currentCursorShape = Qt::ArrowCursor;
    bool cursorHidden = false;
    CursorOverlaySnapshot cursorOverlaySnapshot;

    QOpenGLShaderProgram horizonProgram;
    QOpenGLBuffer horizonVbo { QOpenGLBuffer::VertexBuffer };
    QOpenGLVertexArrayObject horizonVao;
    bool horizonReady = false;

    QColor skyColor = QColor(179, 210, 240);
    QColor groundColor = QColor(188, 206, 188);
    QColor horizonLineColor = QColor(140, 158, 140);

    bool autoFrameOnGeometryChange = true;
    bool autoFramePending = true;
    std::uint64_t lastGeometryRevision = 0;
    bool autoFocusSelection = true;
    QVariantAnimation cameraAnimator;
    CameraKeyframe animationStartState;
    CameraKeyframe animationEndState;
};

