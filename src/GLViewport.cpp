#include "GLViewport.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QMatrix4x4>
#include <QVector4D>
#include <QVector2D>
#include <QOpenGLShader>
#include <QResizeEvent>

#include <QOpenGLContext>
#include <QCursor>

#include <QByteArray>
#include <QFont>
#include <QStringList>
#include <QtMath>
#include <limits>

#include <algorithm>
#include <cmath>
#include <array>
#include <vector>

#include "Tools/ToolManager.h"
#include "NavigationPreferences.h"
#include "CameraNavigation.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"
#include "GeometryKernel/Vector3.h"
#include "GeometryKernel/TransformUtils.h"
#include "Interaction/InferenceEngine.h"
#include "SunModel.h"

#include "Scene/SectionPlane.h"


namespace {

struct HorizonVertex {
    QVector2D position;
    QVector4D color;
};

constexpr const char* kHorizonVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec4 a_color;
out vec4 v_color;
void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_color = a_color;
}
)";

constexpr const char* kHorizonFragmentShader = R"(
#version 330 core
in vec4 v_color;
out vec4 fragColor;
void main() {
    fragColor = v_color;
}
)";

Qt::CursorShape cursorShapeForTool(const Tool* tool)
{
    if (!tool)
        return Qt::ArrowCursor;

    const QString name = QString::fromLatin1(tool->getName());

    if (name == QLatin1String("PanTool")) {
        return tool->getState() == Tool::State::Active ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
    }
    if (name == QLatin1String("OrbitTool")) {
        return Qt::SizeAllCursor;
    }
    if (name == QLatin1String("ZoomTool")) {
        return Qt::PointingHandCursor;
    }
    if (name == QLatin1String("MoveTool")) {
        return Qt::SizeAllCursor;
    }
    if (name == QLatin1String("RotateTool")) {
        return Qt::SizeBDiagCursor;
    }
    if (name == QLatin1String("ScaleTool")) {
        return Qt::SizeFDiagCursor;
    }
    if (name == QLatin1String("PaintBucket")) {
        return Qt::PointingHandCursor;
    }
    if (name == QLatin1String("Text")) {
        return Qt::IBeamCursor;
    }
    if (name == QLatin1String("SmartSelectTool")) {
        return Qt::ArrowCursor;
    }

    static const auto isCrosshairTool = [](const QString& toolName) {
        return toolName == QLatin1String("LineTool")
            || toolName == QLatin1String("Arc")
            || toolName == QLatin1String("Circle")
            || toolName == QLatin1String("Polygon")
            || toolName == QLatin1String("RotatedRectangle")
            || toolName == QLatin1String("Freehand")
            || toolName == QLatin1String("Offset")
            || toolName == QLatin1String("PushPull")
            || toolName == QLatin1String("FollowMe")
            || toolName == QLatin1String("ExtrudeTool")
            || toolName == QLatin1String("Dimension")
            || toolName == QLatin1String("TapeMeasure")
            || toolName == QLatin1String("Protractor")
            || toolName == QLatin1String("Axes")
            || toolName == QLatin1String("SectionTool");
    };

    if (isCrosshairTool(name)) {
        return Qt::CrossCursor;
    }

    return Qt::ArrowCursor;
}

}

GLViewport::GLViewport(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setObjectName(QStringLiteral("MainViewport"));
    setFocusPolicy(Qt::StrongFocus);
    // small timer to keep UI responsive during drags
    repaintTimer.setInterval(16);
    connect(&repaintTimer, &QTimer::timeout, this, QOverload<>::of(&GLViewport::update));
    repaintTimer.start();
    frameTimer.start();
    paletteColors = PalettePreferences::colorsFromState(document.settings().palette());
    setCursor(currentCursorShape);
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(static_cast<float>(skyColor.redF()),
                 static_cast<float>(skyColor.greenF()),
                 static_cast<float>(skyColor.blueF()),
                 1.0f);
    renderer.initialize(context()->extraFunctions());
    initializeHorizonBand();
}

void GLViewport::resizeGL(int w, int h)
{
    glViewport(0,0,w,h);
    if (toolManager) {
        const auto ratio = devicePixelRatioF();
        const int pixelW = std::max(1, static_cast<int>(std::lround(w * ratio)));
        const int pixelH = std::max(1, static_cast<int>(std::lround(h * ratio)));
        toolManager->setViewportSize(pixelW, pixelH);
    }
}

void GLViewport::resizeEvent(QResizeEvent* event)
{
    QOpenGLWidget::resizeEvent(event);
    emit viewportResized(event->size());
}

void GLViewport::setToolManager(ToolManager* manager)
{
    toolManager = manager;
    if (toolManager) {
        const auto ratio = devicePixelRatioF();
        const int pixelW = std::max(1, static_cast<int>(std::lround(width() * ratio)));
        const int pixelH = std::max(1, static_cast<int>(std::lround(height() * ratio)));
        toolManager->setViewportSize(pixelW, pixelH);
        toolManager->setNavigationConfig(navigationConfig);
    }
}

void GLViewport::setNavigationPreferences(NavigationPreferences* prefs)
{
    if (navigationPrefs == prefs)
        return;
    if (navigationPrefs)
        disconnect(navigationPrefs, nullptr, this, nullptr);
    navigationPrefs = prefs;
    if (navigationPrefs) {
        navigationConfig = navigationPrefs->config();
        connect(navigationPrefs, &NavigationPreferences::configChanged, this, [this]() {
            navigationConfig = navigationPrefs->config();
            if (toolManager)
                toolManager->setNavigationConfig(navigationConfig);
        });
    }
    if (toolManager)
        toolManager->setNavigationConfig(navigationConfig);
}

void GLViewport::setPalettePreferences(PalettePreferences* prefs)
{
    if (palettePrefs == prefs)
        return;
    if (palettePrefs)
        disconnect(palettePrefs, nullptr, this, nullptr);
    palettePrefs = prefs;
    if (palettePrefs) {
        paletteColors = palettePrefs->activeColors();
        connect(palettePrefs,
                &PalettePreferences::paletteChanged,
                this,
                [this](const PalettePreferences::ColorSet& colors) {
                    paletteColors = colors;
                    update();
                });
    } else {
        paletteColors = PalettePreferences::colorsFromState(document.settings().palette());
    }
    update();
}

void GLViewport::setRenderStyle(Renderer::RenderStyle style)
{
    if (renderStyle == style) {
        return;
    }
    renderStyle = style;
    update();
}

void GLViewport::setShowHiddenGeometry(bool show)
{
    if (showHiddenGeometry == show) {
        return;
    }
    showHiddenGeometry = show;
    update();
}

void GLViewport::setShowGrid(bool show)
{
    if (gridVisible == show) {
        return;
    }
    gridVisible = show;
    update();
}

void GLViewport::setFrameStatsVisible(bool visible)
{
    if (frameStatsHudVisible == visible)
        return;

    frameStatsHudVisible = visible;
    update();
}

void GLViewport::setSunSettings(const SunSettings& settings)
{
    environmentSettings = settings;
    update();
}

void GLViewport::setBackgroundPalette(const QColor& sky, const QColor& ground, const QColor& horizonLine)
{
    skyColor = sky;
    groundColor = ground;
    horizonLineColor = horizonLine;

    if (context()) {
        makeCurrent();
        glClearColor(static_cast<float>(skyColor.redF()),
                     static_cast<float>(skyColor.greenF()),
                     static_cast<float>(skyColor.blueF()),
                     1.0f);
        initializeHorizonBand();
        doneCurrent();
    }

    update();
}

void GLViewport::setProjectionMode(CameraController::ProjectionMode mode)
{
    if (camera.getProjectionMode() == mode)
        return;
    camera.setProjectionMode(mode);
    update();
}

void GLViewport::toggleProjectionMode()
{
    camera.toggleProjectionMode();
    update();
}

void GLViewport::setFieldOfView(float degrees)
{
    camera.setFieldOfView(degrees);
    update();
}

void GLViewport::setOrthoHeight(float height)
{
    camera.setOrthoHeight(height);
    update();
}

bool GLViewport::applyViewPreset(ViewPresetManager::StandardView view)
{
    if (!viewPresets.applyPreset(view, camera))
        return false;
    activePresetId = viewPresets.idFor(view);
    update();
    return true;
}

bool GLViewport::applyViewPreset(const QString& id)
{
    if (!viewPresets.applyPreset(id, camera))
        return false;
    if (auto mapped = viewPresets.viewForId(id))
        activePresetId = viewPresets.idFor(*mapped);
    else
        activePresetId = id;
    update();
    return true;
}

QString GLViewport::currentViewPresetLabel() const
{
    return viewPresets.labelForId(activePresetId);
}

void GLViewport::paintGL()
{
    currentDrawCalls = 0;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawHorizonBand();

    float aspect = width() > 0 ? float(width()) / float(height() > 0 ? height() : 1) : 1.0f;
    QMatrix4x4 projectionMatrix = buildProjectionMatrix(aspect);

    QMatrix4x4 viewMatrix = buildViewMatrix();

    SunModel::Result sunResult = SunModel::computeSunDirection(environmentSettings.elevationDegrees,
                                                               environmentSettings.azimuthDegrees);
    Renderer::LightingOptions lightingOptions;
    lightingOptions.sunDirection = sunResult.valid ? sunResult.direction : QVector3D(0.3f, 0.8f, 0.6f);
    lightingOptions.sunValid = sunResult.valid;
    lightingOptions.shadowsEnabled = environmentSettings.shadowsEnabled && sunResult.valid;
    lightingOptions.shadowBias = environmentSettings.shadowBias;
    lightingOptions.shadowStrength = environmentSettings.shadowStrength;
    switch (environmentSettings.shadowQuality) {
    case SunSettings::ShadowQuality::Low:
        lightingOptions.shadowMapResolution = 512;
        lightingOptions.shadowSampleRadius = 0;
        break;
    case SunSettings::ShadowQuality::Medium:
        lightingOptions.shadowMapResolution = 1024;
        lightingOptions.shadowSampleRadius = 1;
        break;
    case SunSettings::ShadowQuality::High:
        lightingOptions.shadowMapResolution = 2048;
        lightingOptions.shadowSampleRadius = 2;
        break;
    }
    renderer.setLightingOptions(lightingOptions);

    if (toolManager) {
        ToolInferenceUpdateRequest request;
        if (hasDeviceMouse) {
            QVector3D rayOrigin;
            QVector3D rayDirection;
            if (computePickRay(lastDeviceMouse, rayOrigin, rayDirection)) {
                request.hasRay = true;
                request.ray.origin = Vector3(rayOrigin.x(), rayOrigin.y(), rayOrigin.z());
                request.ray.direction = Vector3(rayDirection.x(), rayDirection.y(), rayDirection.z());
                request.pixelRadius = 6.0f;
            }
        }
        toolManager->updateInference(request);
    }

    renderer.beginFrame(projectionMatrix, viewMatrix, renderStyle);
    drawGrid();
    drawAxes();
    drawSceneGeometry();
    currentDrawCalls += renderer.flush();

    renderer.beginFrame(projectionMatrix, viewMatrix, renderStyle);
    drawSceneOverlays();
    currentDrawCalls += renderer.flush();

    qint64 nanos = frameTimer.nsecsElapsed();
    frameTimer.restart();
    double frameMs = nanos / 1'000'000.0;
    if (smoothedFrameMs <= 0.0)
        smoothedFrameMs = frameMs;
    else
        smoothedFrameMs = smoothedFrameMs * 0.9 + frameMs * 0.1;
    smoothedFps = smoothedFrameMs > 0.0 ? 1000.0 / smoothedFrameMs : 0.0;
    lastDrawCalls = currentDrawCalls;

    emit frameStatsUpdated(smoothedFps, smoothedFrameMs, lastDrawCalls);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawInferenceOverlay(painter, projectionMatrix, viewMatrix);

    if (frameStatsHudVisible) {
        const QString statsText = tr("FPS: %1\nFrame: %2 ms\nDraw Calls: %3")
                                      .arg(smoothedFps, 0, 'f', 1)
                                      .arg(smoothedFrameMs, 0, 'f', 2)
                                      .arg(lastDrawCalls);
        const QStringList lines = statsText.split('\n');
        const QFontMetrics metrics = painter.fontMetrics();
        int textWidth = 0;
        for (const QString& line : lines)
            textWidth = std::max(textWidth, metrics.horizontalAdvance(line));
        const int lineHeight = metrics.lineSpacing();
        const int textHeight = lineHeight * lines.size();
        const int padding = 12;
        const int margin = 12;

        QRect hudRect(0, 0, textWidth + padding * 2, textHeight + padding * 2);
        hudRect.moveBottomRight(QPoint(width() - margin, height() - margin));

        painter.setPen(QColor(206, 214, 224));
        painter.setBrush(QColor(255, 255, 255, 225));
        painter.setOpacity(0.9);
        painter.drawRoundedRect(hudRect, 10, 10);
        painter.setOpacity(1.0);
        painter.setPen(QColor(60, 72, 86));
        painter.drawText(hudRect.adjusted(padding, padding, -padding, -padding),
                         Qt::AlignLeft | Qt::AlignTop,
                         statsText);
    }

    refreshCursorShape();
}

void GLViewport::drawAxes()
{
    const float axisLength = 1.5f;
    const float tickSpacing = 0.5f;
    const float tickSize = 0.08f;
    const QVector4D xColor(0.93f, 0.18f, 0.18f, 1.0f);
    const QVector4D yColor(0.10f, 0.68f, 0.21f, 1.0f);
    const QVector4D zColor(0.16f, 0.44f, 0.91f, 1.0f);

    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(0, 0, 0), QVector3D(axisLength, 0, 0) }, xColor, 3.0f, true, false);
    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(0, 0, 0), QVector3D(0, axisLength, 0) }, yColor, 3.0f, true, false);
    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(0, 0, 0), QVector3D(0, 0, axisLength) }, zColor, 3.0f, true, false);

    std::vector<QVector3D> tickSegments;
    for (float t = tickSpacing; t <= axisLength; t += tickSpacing) {
        tickSegments.push_back(QVector3D(t, 0, -tickSize));
        tickSegments.push_back(QVector3D(t, 0, tickSize));
    }
    if (!tickSegments.empty())
        renderer.addLineSegments(tickSegments, xColor, 2.0f, true, false);

    tickSegments.clear();
    for (float t = tickSpacing; t <= axisLength; t += tickSpacing) {
        tickSegments.push_back(QVector3D(-tickSize, t, 0));
        tickSegments.push_back(QVector3D(tickSize, t, 0));
    }
    if (!tickSegments.empty())
        renderer.addLineSegments(tickSegments, yColor, 2.0f, true, false);

    tickSegments.clear();
    for (float t = tickSpacing; t <= axisLength; t += tickSpacing) {
        tickSegments.push_back(QVector3D(-tickSize, 0, t));
        tickSegments.push_back(QVector3D(tickSize, 0, t));
    }
    if (!tickSegments.empty())
        renderer.addLineSegments(tickSegments, zColor, 2.0f, true, false);
}

void GLViewport::drawGrid()
{
    const Scene::SceneSettings::GridSettings& gridSettings = document.settings().grid();
    const float majorSpacing = std::max(0.001f, gridSettings.majorSpacing);
    const int minorDivisions = std::max(1, gridSettings.minorDivisions);
    const int majorExtent = std::max(1, gridSettings.majorExtent);
    const float minorSpacing = majorSpacing / static_cast<float>(minorDivisions);
    const float gridExtent = majorSpacing * static_cast<float>(majorExtent);
    const float fadeScale = 0.035f;

    const QVector4D axisXColor(0.93f, 0.18f, 0.18f, 1.0f);
    const QVector4D axisZColor(0.16f, 0.44f, 0.91f, 1.0f);

    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(-gridExtent, 0.0f, 0.0f), QVector3D(gridExtent, 0.0f, 0.0f) },
                             axisXColor,
                             2.4f,
                             true,
                             false,
                             Renderer::LineCategory::Generic);
    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(0.0f, 0.0f, -gridExtent), QVector3D(0.0f, 0.0f, gridExtent) },
                             axisZColor,
                             2.4f,
                             true,
                             false,
                             Renderer::LineCategory::Generic);

    if (!gridVisible)
        return;

    const QVector3D majorColor(0.70f, 0.74f, 0.78f);
    const QVector3D minorColor(0.70f, 0.74f, 0.78f);
    const float majorAlpha = 0.55f;
    const float minorAlpha = 0.25f;
    const float majorWidth = 1.35f;
    const float minorWidth = 1.05f;

    const int totalSteps = majorExtent * minorDivisions;

    auto addLine = [&](const QVector3D& a, const QVector3D& b, float distance, bool major) {
        const QVector3D& rgb = major ? majorColor : minorColor;
        float alpha = (major ? majorAlpha : minorAlpha) * std::exp(-std::abs(distance) * fadeScale);
        if (alpha < 0.02f)
            return;
        renderer.addLineSegments(std::vector<QVector3D>{ a, b },
                                 QVector4D(rgb.x(), rgb.y(), rgb.z(), alpha),
                                 major ? majorWidth : minorWidth,
                                 true,
                                 true,
                                 Renderer::LineCategory::Generic,
                                 false,
                                 12.0f);
    };

    for (int i = -totalSteps; i <= totalSteps; ++i) {
        if (i == 0)
            continue;
        const bool major = (i % minorDivisions) == 0;
        const float coord = i * minorSpacing;

        addLine(QVector3D(coord, 0.0f, -gridExtent), QVector3D(coord, 0.0f, gridExtent), coord, major);
        addLine(QVector3D(-gridExtent, 0.0f, coord), QVector3D(gridExtent, 0.0f, coord), coord, major);
    }
}

void GLViewport::initializeHorizonBand()
{
    horizonReady = false;
    horizonProgram.removeAllShaders();

    if (!horizonProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, kHorizonVertexShader))
        return;
    if (!horizonProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kHorizonFragmentShader))
        return;
    if (!horizonProgram.link())
        return;

    if (!horizonVbo.isCreated() && !horizonVbo.create())
        return;

    const float horizonHeight = -0.18f;
    const QVector4D horizonColor(static_cast<float>(horizonLineColor.redF()),
                                 static_cast<float>(horizonLineColor.greenF()),
                                 static_cast<float>(horizonLineColor.blueF()),
                                 1.0f);
    const QVector4D groundColorVec(static_cast<float>(groundColor.redF()),
                                   static_cast<float>(groundColor.greenF()),
                                   static_cast<float>(groundColor.blueF()),
                                   1.0f);
    std::array<HorizonVertex, 4> vertices = {
        HorizonVertex{ QVector2D(-1.0f, horizonHeight), horizonColor },
        HorizonVertex{ QVector2D(1.0f, horizonHeight), horizonColor },
        HorizonVertex{ QVector2D(-1.0f, -1.0f), groundColorVec },
        HorizonVertex{ QVector2D(1.0f, -1.0f), groundColorVec }
    };

    horizonVbo.bind();
    horizonVbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(HorizonVertex)));
    horizonVbo.release();

    if (!horizonVao.isCreated() && !horizonVao.create())
        return;

    QOpenGLVertexArrayObject::Binder binder(&horizonVao);
    horizonVbo.bind();
    horizonProgram.bind();
    horizonProgram.enableAttributeArray(0);
    horizonProgram.setAttributeBuffer(0, GL_FLOAT, offsetof(HorizonVertex, position), 2, sizeof(HorizonVertex));
    horizonProgram.enableAttributeArray(1);
    horizonProgram.setAttributeBuffer(1, GL_FLOAT, offsetof(HorizonVertex, color), 4, sizeof(HorizonVertex));
    horizonProgram.release();
    horizonVbo.release();

    horizonReady = true;
}

void GLViewport::drawHorizonBand()
{
    if (!horizonReady)
        return;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    QOpenGLVertexArrayObject::Binder binder(&horizonVao);
    horizonProgram.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, 2);
    horizonProgram.release();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

namespace {

constexpr float kGhostAlpha = 0.35f;

QVector3D toQt(const Vector3& v)
{
    return QVector3D(v.x, v.y, v.z);
}

std::vector<int> collectFaceLoop(const HalfEdgeMesh& mesh, size_t faceIndex)
{
    std::vector<int> indices;
    const auto& faces = mesh.getFaces();
    if (faceIndex >= faces.size()) {
        return indices;
    }
    int start = faces[faceIndex].halfEdge;
    if (start < 0) {
        return indices;
    }
    const auto& halfEdges = mesh.getHalfEdges();
    int current = start;
    do {
        if (current < 0 || current >= static_cast<int>(halfEdges.size())) {
            indices.clear();
            break;
        }
        const HalfEdgeRecord& edge = halfEdges[current];
        indices.push_back(edge.origin);
        current = edge.next;
    } while (current != start && current != -1);
    return indices;
}

Tool::ModifierState toModifierState(Qt::KeyboardModifiers mods)
{
    Tool::ModifierState state;
    state.shift = mods.testFlag(Qt::ShiftModifier);
    state.ctrl = mods.testFlag(Qt::ControlModifier) || mods.testFlag(Qt::MetaModifier);
    state.alt = mods.testFlag(Qt::AltModifier);
    return state;
}

Vector3 applyGhostTransform(const Vector3& position, const Tool::PreviewGhost& ghost)
{
    Vector3 result = position;
    const float eps = 1e-6f;

    if (ghost.usePivot) {
        Vector3 relative = result - ghost.pivot;
        if (std::fabs(ghost.scale.x - 1.0f) > eps || std::fabs(ghost.scale.y - 1.0f) > eps || std::fabs(ghost.scale.z - 1.0f) > eps) {
            relative.x *= ghost.scale.x;
            relative.y *= ghost.scale.y;
            relative.z *= ghost.scale.z;
        }
        if (std::fabs(ghost.rotationAngle) > eps && ghost.rotationAxis.lengthSquared() > eps) {
            Vector3 scaled = ghost.pivot + relative;
            result = GeometryTransforms::rotateAroundAxis(scaled, ghost.pivot, ghost.rotationAxis, ghost.rotationAngle);
        } else {
            result = ghost.pivot + relative;
        }
    } else {
        if (std::fabs(ghost.scale.x - 1.0f) > eps || std::fabs(ghost.scale.y - 1.0f) > eps || std::fabs(ghost.scale.z - 1.0f) > eps) {
            result.x *= ghost.scale.x;
            result.y *= ghost.scale.y;
            result.z *= ghost.scale.z;
        }
    }

    if (ghost.translation.lengthSquared() > eps) {
        result += ghost.translation;
    }

    return result;
}

float luminance(const QVector4D& color)
{
    return std::clamp(0.2126f * color.x() + 0.7152f * color.y() + 0.0722f * color.z(), 0.0f, 1.0f);
}

QVector4D grayscaleWithRange(const QVector4D& color, float minValue, float maxValue)
{
    float tone = luminance(color);
    tone = std::clamp(tone, minValue, maxValue);
    return QVector4D(tone, tone, tone, color.w());
}

void drawCurve(Renderer& renderer,
               const Curve& curve,
               bool selected,
               bool treatAsHidden,
               Renderer::RenderStyle style,
               const PalettePreferences::ColorSet& palette)
{
    const auto& pts = curve.getBoundaryLoop();
    if (pts.size() < 2) {
        return;
    }
    std::vector<QVector3D> positions;
    positions.reserve(pts.size());
    for (const auto& p : pts) {
        positions.push_back(toQt(p));
    }
    QVector4D color;
    float width = selected ? 3.0f : 2.0f;
    if (treatAsHidden) {
        color = selected ? palette.hiddenCurveSelected : palette.hiddenCurve;
        width = selected ? 2.0f : 1.8f;
    } else if (style == Renderer::RenderStyle::Monochrome) {
        QVector4D base = selected ? palette.curveSelected : palette.curve;
        color = grayscaleWithRange(base, selected ? 0.18f : 0.12f, selected ? 0.3f : 0.2f);
        width = selected ? 2.6f : 1.8f;
    } else if (style == Renderer::RenderStyle::HiddenLine) {
        QVector4D base = selected ? palette.curveSelected : palette.curve;
        color = grayscaleWithRange(base, selected ? 0.18f : 0.12f, selected ? 0.28f : 0.18f);
        width = selected ? 2.4f : 1.6f;
    } else {
        color = selected ? palette.curveSelected : palette.curve;
    }
    bool depthTest = !treatAsHidden;
    bool blend = treatAsHidden;
    Renderer::LineCategory category = treatAsHidden ? Renderer::LineCategory::HiddenEdge : Renderer::LineCategory::Generic;
    renderer.addLineStrip(positions,
                          color,
                          width,
                          false,
                          depthTest,
                          blend,
                          category,
                          treatAsHidden,
                          6.0f);
}

void drawSolid(Renderer& renderer,
               const Solid& solid,
               bool selected,
               bool treatAsHidden,
               Renderer::RenderStyle style,
               const PalettePreferences::ColorSet& palette)
{
    const HalfEdgeMesh& mesh = solid.getMesh();
    const auto& vertices = mesh.getVertices();
    const auto& triangles = mesh.getTriangles();

    QVector4D fillColor = selected ? palette.fillSelected : palette.fill;
    if (style == Renderer::RenderStyle::Monochrome) {
        fillColor = grayscaleWithRange(fillColor, selected ? 0.8f : 0.7f, selected ? 0.95f : 0.9f);
    } else if (style == Renderer::RenderStyle::HiddenLine) {
        fillColor = grayscaleWithRange(fillColor, 0.72f, 0.9f);
    }
    if (!treatAsHidden) {
        for (const auto& tri : triangles) {
            if (tri.v0 < 0 || tri.v1 < 0 || tri.v2 < 0) {
                continue;
            }
            if (tri.v0 >= static_cast<int>(vertices.size()) ||
                tri.v1 >= static_cast<int>(vertices.size()) ||
                tri.v2 >= static_cast<int>(vertices.size())) {
                continue;
            }
            QVector3D normal = toQt(tri.normal);
            renderer.addTriangle(
                toQt(vertices[(size_t)tri.v0].position),
                toQt(vertices[(size_t)tri.v1].position),
                toQt(vertices[(size_t)tri.v2].position),
                normal,
                fillColor);
        }
    }

    QVector4D edgeColor = selected ? palette.edgeSelected : palette.edge;
    if (style == Renderer::RenderStyle::Monochrome) {
        edgeColor = grayscaleWithRange(edgeColor, selected ? 0.2f : 0.13f, selected ? 0.32f : 0.19f);
    } else if (style == Renderer::RenderStyle::HiddenLine) {
        edgeColor = grayscaleWithRange(edgeColor, selected ? 0.2f : 0.12f, selected ? 0.3f : 0.18f);
    }
    if (treatAsHidden) {
        edgeColor = selected ? palette.hiddenEdgeSelected : palette.hiddenEdge;
    }
    const auto& faces = mesh.getFaces();
    for (size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        auto loop = collectFaceLoop(mesh, faceIndex);
        if (loop.size() < 2) {
            continue;
        }
        std::vector<QVector3D> positions;
        positions.reserve(loop.size());
        for (int idx : loop) {
            if (idx < 0 || idx >= static_cast<int>(vertices.size())) {
                positions.clear();
                break;
            }
            positions.push_back(toQt(vertices[(size_t)idx].position));
        }
        if (positions.size() < 2) {
            continue;
        }
        if (style == Renderer::RenderStyle::HiddenLine && !treatAsHidden) {
            QVector4D hiddenEdgeColor = selected ? palette.hiddenCurveSelected : palette.hiddenCurve;
            hiddenEdgeColor.setW(1.0f);
            renderer.addLineStrip(positions,
                                  hiddenEdgeColor,
                                  selected ? 1.9f : 1.4f,
                                  true,
                                  false,
                                  false,
                                  Renderer::LineCategory::HiddenEdge,
                                  true,
                                  6.5f);
        }
        bool depthTest = !treatAsHidden;
        bool blend = treatAsHidden;
        Renderer::LineCategory category = treatAsHidden ? Renderer::LineCategory::HiddenEdge : Renderer::LineCategory::Edge;
        float width = treatAsHidden ? 1.6f : (selected ? 2.2f : 1.5f);
        renderer.addLineStrip(positions,
                              edgeColor,
                              width,
                              true,
                              depthTest,
                              blend,
                              category,
                              treatAsHidden,
                              6.5f);
    }
}

void drawGhostCurve(Renderer& renderer,
                    const Curve& curve,
                    const Tool::PreviewGhost& ghost,
                    const PalettePreferences::ColorSet& palette)
{
    const auto& pts = curve.getBoundaryLoop();
    if (pts.size() < 2) {
        return;
    }
    std::vector<QVector3D> positions;
    positions.reserve(pts.size());
    for (const auto& p : pts) {
        Vector3 transformed = applyGhostTransform(p, ghost);
        positions.push_back(toQt(transformed));
    }
    QVector4D ghostColor = palette.highlight;
    ghostColor.setW(kGhostAlpha);
    renderer.addLineStrip(positions, ghostColor, 2.4f, false, true, true);
}

void drawGhostSolid(Renderer& renderer,
                    const Solid& solid,
                    const Tool::PreviewGhost& ghost,
                    const PalettePreferences::ColorSet& palette)
{
    const HalfEdgeMesh& mesh = solid.getMesh();
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();

    for (size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        auto loop = collectFaceLoop(mesh, faceIndex);
        if (loop.size() < 2) {
            continue;
        }
        std::vector<QVector3D> positions;
        positions.reserve(loop.size());
        bool valid = true;
        for (int idx : loop) {
            if (idx < 0 || idx >= static_cast<int>(vertices.size())) {
                valid = false;
                break;
            }
            Vector3 transformed = applyGhostTransform(vertices[(size_t)idx].position, ghost);
            positions.push_back(toQt(transformed));
        }
        if (!valid || positions.size() < 2) {
            continue;
        }
        QVector4D ghostColor = palette.highlight;
        ghostColor.setW(kGhostAlpha);
        renderer.addLineStrip(positions, ghostColor, 2.0f, true, true, true);
    }
}

}

void GLViewport::drawSceneGeometry()
{
    std::vector<QVector4D> clipPlanes;
    const auto& planes = document.sectionPlanes();
    clipPlanes.reserve(planes.size());
    for (const auto& plane : planes) {
        if (!plane.isActive()) {
            continue;
        }
        const Vector3& normal = plane.getNormal();
        QVector3D qNormal(normal.x, normal.y, normal.z);
        if (qNormal.lengthSquared() <= 1e-6f) {
            continue;
        }
        clipPlanes.emplace_back(normal.x, normal.y, normal.z, plane.getOffset());
    }
    renderer.setClipPlanes(clipPlanes);

    const auto& objs = document.geometry().getObjects();
    for (const auto& uptr : objs) {
        if (!uptr->isVisible()) {
            continue;
        }
        const bool hidden = uptr->isHidden();
        if (hidden && !showHiddenGeometry) {
            continue;
        }
        const bool treatAsHidden = hidden && showHiddenGeometry;
        if (uptr->getType() == ObjectType::Curve) {
            drawCurve(renderer,
                      *static_cast<const Curve*>(uptr.get()),
                      uptr->isSelected(),
                      treatAsHidden,
                      renderStyle,
                      paletteColors);
        } else if (uptr->getType() == ObjectType::Solid) {
            drawSolid(renderer,
                      *static_cast<const Solid*>(uptr.get()),
                      uptr->isSelected(),
                      treatAsHidden,
                      renderStyle,
                      paletteColors);
        }
    }
}

void GLViewport::drawSceneOverlays()
{
    renderer.setClipPlanes(std::vector<QVector4D>{});

    const Scene::SceneSettings& settings = document.settings();
    if (settings.sectionPlanesVisible()) {
        const auto& planes = document.sectionPlanes();
        for (const auto& plane : planes) {
            if (!plane.isVisible()) {
                continue;
            }

            const auto& transform = plane.getTransform();
            QMatrix4x4 basis(transform.data());
            QVector3D origin = basis.column(3).toVector3D();
            QVector3D xAxis = basis.column(0).toVector3D();
            QVector3D yAxis = basis.column(1).toVector3D();
            QVector3D normalVec = QVector3D(plane.getNormal().x, plane.getNormal().y, plane.getNormal().z);
            if (xAxis.lengthSquared() < 1e-6f) {
                xAxis = QVector3D(1.0f, 0.0f, 0.0f);
            }
            if (yAxis.lengthSquared() < 1e-6f) {
                yAxis = QVector3D(0.0f, 0.0f, 1.0f);
            }
            if (normalVec.lengthSquared() < 1e-6f) {
                normalVec = QVector3D::crossProduct(xAxis, yAxis);
            }
            xAxis.normalize();
            yAxis.normalize();
            normalVec.normalize();

            const Scene::SectionFillStyle& style = plane.fillStyle();
            float extent = std::max(0.25f, style.extent);
            QVector3D right = xAxis * extent;
            QVector3D up = yAxis * extent;

            std::array<QVector3D, 4> corners = {
                origin + right + up,
                origin - right + up,
                origin - right - up,
                origin + right - up
            };

            if (settings.sectionFillsVisible() && style.fillEnabled) {
                QVector4D fillColor(style.red, style.green, style.blue, style.alpha);
                renderer.addTriangle(corners[0], corners[1], corners[2], normalVec, fillColor);
                renderer.addTriangle(corners[0], corners[2], corners[3], normalVec, fillColor);
            }

            QVector4D outlineColor(style.red * 0.8f, style.green * 0.8f, style.blue * 0.8f, 0.95f);
            std::vector<QVector3D> outline = { corners[0], corners[1], corners[2], corners[3] };
            renderer.addLineStrip(outline,
                                  outlineColor,
                                  1.6f,
                                  true,
                                  false,
                                  true,
                                  Renderer::LineCategory::Generic,
                                  false,
                                  8.0f);

            QVector4D handleColor(0.35f, 0.47f, 0.9f, 0.9f);
            std::vector<QVector3D> axisSegments = {
                origin - right,
                origin + right,
                origin - up,
                origin + up
            };
            renderer.addLineSegments(axisSegments,
                                     handleColor,
                                     1.8f,
                                     false,
                                     true,
                                     Renderer::LineCategory::Generic,
                                     false,
                                     8.0f);

            QVector4D arrowColor(0.96f, 0.42f, 0.18f, 1.0f);
            float arrowLength = extent * 1.35f;
            QVector3D arrowEnd = origin + normalVec * arrowLength;
            renderer.addLineSegments({ origin, arrowEnd },
                                     arrowColor,
                                     2.2f,
                                     false,
                                     true,
                                     Renderer::LineCategory::Generic,
                                     false,
                                     8.0f);

            QVector3D arrowSide = QVector3D::crossProduct(normalVec, xAxis);
            if (arrowSide.lengthSquared() < 1e-6f) {
                arrowSide = QVector3D::crossProduct(normalVec, yAxis);
            }
            if (arrowSide.lengthSquared() > 1e-6f) {
                arrowSide.normalize();
                float headSize = extent * 0.35f;
                QVector3D headBase = arrowEnd - normalVec * (headSize * 0.75f);
                std::vector<QVector3D> headSegments = {
                    arrowEnd,
                    headBase + arrowSide * headSize,
                    arrowEnd,
                    headBase - arrowSide * headSize
                };
                renderer.addLineSegments(headSegments,
                                         arrowColor,
                                         2.0f,
                                         false,
                                         true,
                                         Renderer::LineCategory::Generic,
                                         false,
                                         8.0f);
            }
        }
    }

    if (settings.guidesVisible()) {
        const auto& guides = document.geometry().getGuides();
        std::vector<QVector3D> lineSegments;
        lineSegments.reserve(guides.lines.size() * 2);
        for (const auto& line : guides.lines) {
            lineSegments.emplace_back(line.start.x, line.start.y, line.start.z);
            lineSegments.emplace_back(line.end.x, line.end.y, line.end.z);
        }
        const Scene::SceneSettings::GridSettings& gridSettings = settings.grid();
        const float baseSize = std::max(0.1f, gridSettings.majorSpacing * 0.1f);
        const QVector4D guideLineColor(0.18f, 0.62f, 0.85f, 0.9f);
        if (!lineSegments.empty()) {
            renderer.addLineSegments(lineSegments,
                                     guideLineColor,
                                     1.6f,
                                     false,
                                     true,
                                     Renderer::LineCategory::Generic,
                                     false,
                                     8.0f);
        }

        std::vector<QVector3D> pointSegments;
        pointSegments.reserve(guides.points.size() * 6);
        for (const auto& point : guides.points) {
            QVector3D origin(point.position.x, point.position.y, point.position.z);
            QVector3D offsetX(baseSize, 0.0f, 0.0f);
            QVector3D offsetY(0.0f, baseSize, 0.0f);
            QVector3D offsetZ(0.0f, 0.0f, baseSize);
            pointSegments.push_back(origin - offsetX);
            pointSegments.push_back(origin + offsetX);
            pointSegments.push_back(origin - offsetY);
            pointSegments.push_back(origin + offsetY);
            pointSegments.push_back(origin - offsetZ);
            pointSegments.push_back(origin + offsetZ);
        }
        if (!pointSegments.empty()) {
            renderer.addLineSegments(pointSegments,
                                     QVector4D(0.95f, 0.55f, 0.2f, 0.9f),
                                     1.6f,
                                     false,
                                     true,
                                     Renderer::LineCategory::Generic,
                                     false,
                                     8.0f);
        }

        std::vector<QVector3D> angleSegments;
        angleSegments.reserve(guides.angles.size() * 4);
        for (const auto& angle : guides.angles) {
            QVector3D origin(angle.origin.x, angle.origin.y, angle.origin.z);
            QVector3D start(angle.startDirection.x, angle.startDirection.y, angle.startDirection.z);
            QVector3D end(angle.endDirection.x, angle.endDirection.y, angle.endDirection.z);
            if (!start.isNull())
                start.normalize();
            if (!end.isNull())
                end.normalize();
            const float rayLength = std::max(gridSettings.majorSpacing, baseSize * 4.0f);
            angleSegments.push_back(origin);
            angleSegments.push_back(origin + start * rayLength);
            angleSegments.push_back(origin);
            angleSegments.push_back(origin + end * rayLength);
        }
        if (!angleSegments.empty()) {
            renderer.addLineSegments(angleSegments,
                                     QVector4D(0.6f, 0.3f, 0.9f, 0.85f),
                                     1.8f,
                                     false,
                                     true,
                                     Renderer::LineCategory::Generic,
                                     false,
                                     8.0f);
        }
    }

    Tool::PreviewState preview;
    if (toolManager) {
        if (Tool* active = toolManager->getActiveTool()) {
            preview = active->getPreviewState();
        }
    }

    if (!preview.polylines.empty()) {
        QVector4D color(0.15f, 0.65f, 0.95f, 0.7f);
        for (const auto& poly : preview.polylines) {
            if (poly.points.size() < 2) {
                continue;
            }
            std::vector<QVector3D> positions;
            positions.reserve(poly.points.size());
            for (const auto& p : poly.points) {
                positions.push_back(QVector3D(p.x, p.y, p.z));
            }
            renderer.addLineStrip(positions,
                                  color,
                                  2.0f,
                                  poly.closed,
                                  false,
                                  true,
                                  Renderer::LineCategory::Generic,
                                  false,
                                  8.0f);
        }
    }

    for (const auto& ghost : preview.ghosts) {
        if (!ghost.object) {
            continue;
        }
        if (ghost.object->getType() == ObjectType::Curve) {
            drawGhostCurve(renderer, *static_cast<const Curve*>(ghost.object), ghost, paletteColors);
        } else if (ghost.object->getType() == ObjectType::Solid) {
            drawGhostSolid(renderer, *static_cast<const Solid*>(ghost.object), ghost, paletteColors);
        }
    }
}

std::pair<float, float> GLViewport::depthRangeForAspect(float aspect) const
{
    if (aspect <= 0.0f)
        aspect = 1.0f;

    constexpr float kMinNearAbsolute = 0.01f;
    constexpr float kNearFraction = 0.001f;

    const float focusDistance = std::max(camera.getDistance(), 0.001f);
    const float minNearFromFraction = std::max(focusDistance * kNearFraction, kMinNearAbsolute);
    float nearPlane = minNearFromFraction;
    float farPlane = std::max(focusDistance * 4.0f, nearPlane * 8.0f);
    float frontDistanceLimit = std::numeric_limits<float>::infinity();

    Vector3 minBounds;
    Vector3 maxBounds;
    if (computeBounds(false, minBounds, maxBounds)) {
        float cx, cy, cz;
        camera.getCameraPosition(cx, cy, cz);
        float tx, ty, tz;
        camera.getTarget(tx, ty, tz);

        const QVector3D cameraPos(cx, cy, cz);
        QVector3D viewDir(tx - cx, ty - cy, tz - cz);
        const float viewLength = viewDir.length();
        if (viewLength > 1e-4f)
            viewDir /= viewLength;
        else
            viewDir = QVector3D(0.0f, 0.0f, -1.0f);

        const QVector3D minCorner(minBounds.x, minBounds.y, minBounds.z);
        const QVector3D maxCorner(maxBounds.x, maxBounds.y, maxBounds.z);
        const std::array<QVector3D, 8> corners = {
            QVector3D(minCorner.x(), minCorner.y(), minCorner.z()),
            QVector3D(maxCorner.x(), minCorner.y(), minCorner.z()),
            QVector3D(minCorner.x(), maxCorner.y(), minCorner.z()),
            QVector3D(maxCorner.x(), maxCorner.y(), minCorner.z()),
            QVector3D(minCorner.x(), minCorner.y(), maxCorner.z()),
            QVector3D(maxCorner.x(), minCorner.y(), maxCorner.z()),
            QVector3D(minCorner.x(), maxCorner.y(), maxCorner.z()),
            QVector3D(maxCorner.x(), maxCorner.y(), maxCorner.z())
        };

        float minProj = std::numeric_limits<float>::max();
        float maxProj = -std::numeric_limits<float>::max();
        for (const QVector3D& corner : corners) {
            const float proj = QVector3D::dotProduct(corner - cameraPos, viewDir);
            minProj = std::min(minProj, proj);
            maxProj = std::max(maxProj, proj);
        }

        if (maxProj > 0.0f) {
            const float frontDistance = std::max(minProj, 0.0f);
            const float depthSpan = std::max(maxProj - frontDistance, focusDistance * 0.5f);
            float margin = std::max(frontDistance * 0.1f, nearPlane * 2.0f);
            margin = std::min(margin, depthSpan * 0.25f);
            margin = std::max(margin, nearPlane);

            frontDistanceLimit = frontDistance;

            const float nearCandidate = std::max(frontDistance - margin, 0.0f);
            if (nearCandidate > nearPlane)
                nearPlane = nearCandidate;

            const float farCandidate = maxProj + margin;
            if (farCandidate > farPlane)
                farPlane = farCandidate;
        }
    } else {
        farPlane = std::max(farPlane, 1000.0f);
    }

    if (farPlane <= nearPlane)
        farPlane = nearPlane + std::max(nearPlane * 0.25f, 1.0f);

    if (std::isfinite(frontDistanceLimit)) {
        const float cappedFront = std::max(frontDistanceLimit - 1e-3f, kMinNearAbsolute);
        if (cappedFront >= minNearFromFraction)
            nearPlane = std::clamp(nearPlane, minNearFromFraction, cappedFront);
        else
            nearPlane = std::max(nearPlane, cappedFront);
    } else {
        nearPlane = std::max(nearPlane, minNearFromFraction);
    }

    return { nearPlane, farPlane };
}

QMatrix4x4 GLViewport::buildProjectionMatrix(float aspect) const
{
    QMatrix4x4 projection;
    const auto [znear, zfar] = depthRangeForAspect(aspect);
    float safeAspect = aspect;
    if (safeAspect <= 0.0f)
        safeAspect = 1.0f;
    if (camera.getProjectionMode() == CameraController::ProjectionMode::Parallel) {
        float orthoHeight = camera.getOrthoHeight();
        if (orthoHeight <= 0.0f)
            orthoHeight = 1.0f;
        const float halfHeight = orthoHeight * 0.5f;
        const float halfWidth = halfHeight * safeAspect;
        projection.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, znear, zfar);
    } else {
        float fov = camera.getFieldOfView();
        if (fov < 1.0f)
            fov = 1.0f;
        projection.perspective(fov, safeAspect, znear, zfar);
    }
    return projection;
}

QMatrix4x4 GLViewport::buildViewMatrix() const
{
    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    QMatrix4x4 viewMatrix;
    viewMatrix.lookAt(QVector3D(cx, cy, cz), QVector3D(tx, ty, tz), QVector3D(0.0f, 1.0f, 0.0f));
    return viewMatrix;
}

bool GLViewport::computePickRay(const QPoint& devicePos, QVector3D& origin, QVector3D& direction) const
{
    if (width() <= 0 || height() <= 0) {
        return false;
    }

    const float ratio = devicePixelRatioF();
    const float pixelWidth = std::max(1.0f, static_cast<float>(width()) * ratio);
    const float pixelHeight = std::max(1.0f, static_cast<float>(height()) * ratio);
    float nx = ((static_cast<float>(devicePos.x()) + 0.5f) / pixelWidth) * 2.0f - 1.0f;
    float ny = 1.0f - ((static_cast<float>(devicePos.y()) + 0.5f) / pixelHeight) * 2.0f;

    const float aspect = pixelWidth / pixelHeight;
    QMatrix4x4 projection = buildProjectionMatrix(aspect);
    QMatrix4x4 view = buildViewMatrix();

    bool invertible = false;
    QMatrix4x4 inv = (projection * view).inverted(&invertible);
    if (!invertible) {
        return false;
    }

    QVector4D nearPoint(nx, ny, -1.0f, 1.0f);
    QVector4D farPoint(nx, ny, 1.0f, 1.0f);
    QVector4D worldNear = inv * nearPoint;
    QVector4D worldFar = inv * farPoint;
    if (qFuzzyIsNull(worldNear.w()) || qFuzzyIsNull(worldFar.w())) {
        return false;
    }
    worldNear /= worldNear.w();
    worldFar /= worldFar.w();

    origin = worldNear.toVector3DAffine();
    direction = (worldFar - worldNear).toVector3D();
    if (qFuzzyIsNull(direction.lengthSquared())) {
        return false;
    }
    direction.normalize();
    return true;
}

bool GLViewport::projectWorldToScreen(const QVector3D& world, const QMatrix4x4& projection, const QMatrix4x4& view, QPointF& out) const
{
    QVector4D clip = projection * view * QVector4D(world, 1.0f);
    if (qFuzzyIsNull(clip.w())) {
        return false;
    }
    clip /= clip.w();
    QVector3D ndc = clip.toVector3D();
    if (ndc.z() < 0.0f || ndc.z() > 1.0f) {
        return false;
    }
    float sx = (ndc.x() * 0.5f + 0.5f) * static_cast<float>(width());
    float sy = (-ndc.y() * 0.5f + 0.5f) * static_cast<float>(height());
    out = QPointF(sx, sy);
    return true;
}

void GLViewport::drawInferenceOverlay(QPainter& painter, const QMatrix4x4& projection, const QMatrix4x4& view)
{
    if (!toolManager) {
        return;
    }

    const auto& inference = toolManager->getCurrentInference();
    if (!inference.isValid()) {
        return;
    }

    auto toVector3D = [](const Vector3& v) { return QVector3D(v.x, v.y, v.z); };

    QPointF screenPoint;
    if (!projectWorldToScreen(toVector3D(inference.position), projection, view, screenPoint)) {
        return;
    }

    auto colorForType = [](Interaction::InferenceSnapType type) {
        switch (type) {
        case Interaction::InferenceSnapType::Endpoint: return QColor(216, 57, 49);
        case Interaction::InferenceSnapType::Midpoint: return QColor(38, 132, 255);
        case Interaction::InferenceSnapType::Intersection: return QColor(140, 46, 201);
        case Interaction::InferenceSnapType::FaceCenter: return QColor(255, 176, 29);
        case Interaction::InferenceSnapType::OnEdge: return QColor(26, 188, 156);
        case Interaction::InferenceSnapType::OnFace: return QColor(241, 196, 15);
        case Interaction::InferenceSnapType::Axis: return QColor(52, 152, 219);
        case Interaction::InferenceSnapType::Parallel: return QColor(231, 76, 60);
        case Interaction::InferenceSnapType::Perpendicular: return QColor(155, 89, 182);
        default: return QColor(255, 255, 255);
        }
    };

    QColor color = colorForType(inference.type);
    QColor fill = color;
    fill.setAlpha(180);

    painter.setPen(QPen(color, 1.8f));
    painter.setBrush(fill);
    painter.drawEllipse(screenPoint, 6.0, 6.0);

    const QString label = QString::fromLatin1(Interaction::toString(inference.type)) +
        (inference.locked ? tr(" (locked)") : QString());
    painter.setPen(QPen(Qt::black, 1.0));
    painter.drawText(screenPoint + QPointF(10.0, -10.0), label);

    Vector3 dir = inference.direction;
    if (dir.lengthSquared() > 1e-6f) {
        Vector3 normDir = dir.normalized();
        Vector3 start = inference.position - normDir * 0.4f;
        Vector3 end = inference.position + normDir * 0.4f;
        QPointF startPoint;
        QPointF endPoint;
        if (projectWorldToScreen(toVector3D(start), projection, view, startPoint) &&
            projectWorldToScreen(toVector3D(end), projection, view, endPoint)) {
            QPen pen(color, 1.6f, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
            pen.setDashPattern({ 4.0, 4.0 });
            painter.setPen(pen);
            painter.drawLine(startPoint, endPoint);
        }
    }

    if (inference.type == Interaction::InferenceSnapType::Axis) {
        QPointF anchorPoint;
        if (projectWorldToScreen(toVector3D(inference.reference), projection, view, anchorPoint)) {
            QPen anchorPen(color, 1.2f, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(anchorPen);
            painter.drawLine(anchorPoint, screenPoint);
        }
    }
}

bool GLViewport::computeBounds(bool selectedOnly, Vector3& outMin, Vector3& outMax) const
{
    bool hasBounds = false;
    const auto& objects = document.geometry().getObjects();
    for (const auto& object : objects) {
        if (!object) {
            continue;
        }
        if (selectedOnly && !object->isSelected()) {
            continue;
        }
        const HalfEdgeMesh& mesh = object->getMesh();
        const auto& vertices = mesh.getVertices();
        for (const auto& vertex : vertices) {
            const Vector3& p = vertex.position;
            if (!hasBounds) {
                outMin = p;
                outMax = p;
                hasBounds = true;
            } else {
                outMin.x = std::min(outMin.x, p.x);
                outMin.y = std::min(outMin.y, p.y);
                outMin.z = std::min(outMin.z, p.z);
                outMax.x = std::max(outMax.x, p.x);
                outMax.y = std::max(outMax.y, p.y);
                outMax.z = std::max(outMax.z, p.z);
            }
        }
    }
    return hasBounds;
}

bool GLViewport::applyZoomToBounds(const Vector3& minBounds, const Vector3& maxBounds)
{
    if (!CameraNavigation::frameBounds(camera, minBounds, maxBounds, width(), height()))
        return false;
    update();
    return true;
}

void GLViewport::refreshCursorShape()
{
    if (!underMouse()) {
        return;
    }

    Qt::CursorShape desired = Qt::ArrowCursor;
    if (activeNavigationBinding) {
        desired = Qt::ClosedHandCursor;
    } else if (toolManager) {
        if (Tool* tool = toolManager->getActiveTool()) {
            desired = cursorShapeForTool(tool);
        }
    }

    if (desired != currentCursorShape) {
        setCursor(desired);
        currentCursorShape = desired;
    }
}

void GLViewport::zoomInStep()
{
    camera.zoomCamera(1.0f);
    update();
}

void GLViewport::zoomOutStep()
{
    camera.zoomCamera(-1.0f);
    update();
}

bool GLViewport::zoomExtents()
{
    Vector3 minBounds;
    Vector3 maxBounds;
    if (!computeBounds(false, minBounds, maxBounds)) {
        return false;
    }
    return applyZoomToBounds(minBounds, maxBounds);
}

bool GLViewport::zoomSelection()
{
    Vector3 minBounds;
    Vector3 maxBounds;
    if (!computeBounds(true, minBounds, maxBounds)) {
        return false;
    }
    return applyZoomToBounds(minBounds, maxBounds);
}

void GLViewport::mousePressEvent(QMouseEvent* e)
{
    if (toolManager) {
        toolManager->updatePointerModifiers(toModifierState(e->modifiers()));
    }
    lastMouse = e->pos();
    const auto ratio = devicePixelRatioF();
    const QPointF devicePos = e->position() * ratio;
    lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
    hasDeviceMouse = true;

    if (toolManager) {
        if (auto binding = navigationConfig.matchDrag(e->button(), e->modifiers())) {
            navigationButton = e->button();
            activeNavigationBinding = binding;
            QByteArray nameBytes(binding->toolName.c_str());
            toolManager->activateTool(nameBytes.constData(), binding->temporary);
            Tool::PointerInput input;
            input.x = lastDeviceMouse.x();
            input.y = lastDeviceMouse.y();
            input.modifiers = toModifierState(e->modifiers());
            input.devicePixelRatio = ratio;
            toolManager->updatePointerModifiers(input.modifiers);
            toolManager->handlePointerDown(input);
            update();
            refreshCursorShape();
            return;
        }
    }

    if (toolManager && e->button() == Qt::LeftButton) {
        Tool::PointerInput input;
        input.x = std::lround(devicePos.x());
        input.y = std::lround(devicePos.y());
        input.modifiers = toModifierState(e->modifiers());
        input.devicePixelRatio = ratio;
        toolManager->updatePointerModifiers(input.modifiers);
        toolManager->handlePointerDown(input);
    }

    refreshCursorShape();
}

void GLViewport::mouseMoveEvent(QMouseEvent* e)
{
    lastMouse = e->pos();

    const auto ratio = devicePixelRatioF();
    const QPointF devicePos = e->position() * ratio;
    lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
    hasDeviceMouse = true;

    Tool::PointerInput input;
    input.x = lastDeviceMouse.x();
    input.y = lastDeviceMouse.y();
    input.modifiers = toModifierState(e->modifiers());
    input.devicePixelRatio = ratio;

    bool navigationHandled = false;

    if (toolManager) {
        toolManager->updatePointerModifiers(input.modifiers);
        if (activeNavigationBinding) {
            if (!e->buttons().testFlag(navigationButton)) {
                toolManager->handlePointerUp(input);
                if (activeNavigationBinding->temporary && toolManager) {
                    toolManager->restorePreviousTool();
                }
                activeNavigationBinding.reset();
                navigationButton = Qt::NoButton;
            } else {
                toolManager->handlePointerMove(input);
                navigationHandled = true;
            }
        }

        if (!navigationHandled) {
            toolManager->handlePointerMove(input);
        }
    }

    QVector3D world;
    if (projectCursorToGround(e->position(), world)) {
        emit cursorPositionChanged(world.x(), world.y(), world.z());
    }

    if (navigationHandled)
        update();

    refreshCursorShape();
}

void GLViewport::mouseReleaseEvent(QMouseEvent* e)
{
    if (toolManager) {
        toolManager->updatePointerModifiers(toModifierState(e->modifiers()));
    }
    const auto ratio = devicePixelRatioF();
    const QPointF devicePos = e->position() * ratio;
    lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
    hasDeviceMouse = true;

    Tool::PointerInput input;
    input.x = lastDeviceMouse.x();
    input.y = lastDeviceMouse.y();
    input.modifiers = toModifierState(e->modifiers());
    input.devicePixelRatio = ratio;

    if (activeNavigationBinding && e->button() == navigationButton) {
        if (toolManager) {
            toolManager->handlePointerUp(input);
            if (activeNavigationBinding->temporary)
                toolManager->restorePreviousTool();
        }
        activeNavigationBinding.reset();
        navigationButton = Qt::NoButton;
        update();
        refreshCursorShape();
        return;
    }

    if (toolManager && e->button() == Qt::LeftButton) {
        toolManager->updatePointerModifiers(input.modifiers);
        toolManager->handlePointerUp(input);
    }

    refreshCursorShape();
}

void GLViewport::wheelEvent(QWheelEvent* e)
{
    if (toolManager) {
        toolManager->updatePointerModifiers(toModifierState(e->modifiers()));
    }
    double delta = 0.0;
    if (!e->angleDelta().isNull())
        delta = e->angleDelta().y() / 120.0;
    else if (!e->pixelDelta().isNull())
        delta = e->pixelDelta().y() / 120.0;

    if (qFuzzyIsNull(delta)) {
        QOpenGLWidget::wheelEvent(e);
        return;
    }

    if (navigationConfig.invertWheel)
        delta = -delta;
    delta *= navigationConfig.wheelStep;

    const auto ratio = devicePixelRatioF();
    const QPointF devicePos = e->position() * ratio;
    lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
    hasDeviceMouse = true;

    int pixelW = std::max(1, static_cast<int>(std::lround(width() * ratio)));
    int pixelH = std::max(1, static_cast<int>(std::lround(height() * ratio)));

    CameraNavigation::zoomAboutCursor(camera, static_cast<float>(delta), lastDeviceMouse.x(), lastDeviceMouse.y(), pixelW,
                                      pixelH, navigationConfig.zoomToCursor);
    update();
    refreshCursorShape();
}

void GLViewport::keyPressEvent(QKeyEvent* e)
{
    bool consumed = false;
    if (toolManager) {
        if (e->key() == Qt::Key_Escape) {
            toolManager->cancelActiveTool();
            consumed = true;
        } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            toolManager->commitActiveTool();
            consumed = true;
        }
        if (!consumed)
            toolManager->handleKeyPress(e->key());
    }
    if (!consumed) {
        QOpenGLWidget::keyPressEvent(e);
    }
    update();
    refreshCursorShape();
}

void GLViewport::keyReleaseEvent(QKeyEvent* e)
{
    if (toolManager) {
        toolManager->handleKeyRelease(e->key());
    }
    QOpenGLWidget::keyReleaseEvent(e);
    update();
    refreshCursorShape();
}

void GLViewport::leaveEvent(QEvent* event)
{
    QOpenGLWidget::leaveEvent(event);
    hasDeviceMouse = false;
    if (activeNavigationBinding) {
        if (toolManager && activeNavigationBinding->temporary)
            toolManager->restorePreviousTool();
        activeNavigationBinding.reset();
    }
    navigationButton = Qt::NoButton;
    if (toolManager) {
        toolManager->clearInference();
    }
    unsetCursor();
    currentCursorShape = Qt::ArrowCursor;
    emit cursorPositionChanged(std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::quiet_NaN());
}

bool GLViewport::projectCursorToGround(const QPointF& pos, QVector3D& world) const
{
    if (width() <= 0 || height() <= 0)
        return false;

    const float ratio = devicePixelRatioF();
    QPoint devicePos(std::lround(pos.x() * ratio), std::lround(pos.y() * ratio));

    QVector3D origin;
    QVector3D direction;
    if (!computePickRay(devicePos, origin, direction))
        return false;

    if (qFuzzyIsNull(direction.y()))
        return false;
    const float t = -origin.y() / direction.y();
    if (t < 0.0f)
        return false;

    world = origin + direction * t;
    return true;
}



