#include "GLViewport.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QMatrix4x4>
#include <QVector4D>

#include <QOpenGLContext>

#include <QByteArray>
#include <QFont>
#include <QtMath>
#include <limits>

#include <algorithm>
#include <cmath>
#include <array>

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


GLViewport::GLViewport(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // small timer to keep UI responsive during drags
    repaintTimer.setInterval(16);
    connect(&repaintTimer, &QTimer::timeout, this, QOverload<>::of(&GLViewport::update));
    repaintTimer.start();
    frameTimer.start();
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
    renderer.initialize(context()->extraFunctions());
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

void GLViewport::setSunSettings(const SunSettings& settings)
{
    environmentSettings = settings;
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

    float aspect = width() > 0 ? float(width()) / float(height() > 0 ? height() : 1) : 1.0f;
    QMatrix4x4 projectionMatrix = buildProjectionMatrix(aspect);

    QMatrix4x4 viewMatrix = buildViewMatrix();

    SunModel::Result sunResult = SunModel::computeSunDirection(environmentSettings.date,
                                                      environmentSettings.time,
                                                      environmentSettings.latitude,
                                                      environmentSettings.longitude,
                                                      environmentSettings.effectiveTimezoneMinutes());
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
    drawAxisGizmo(painter, viewMatrix);
    painter.setPen(QColor(231, 234, 240));
    painter.setBrush(QColor(11, 13, 16, 220));
    QRect hudRect(12, 12, 200, 60);
    painter.setOpacity(0.85);
    painter.drawRoundedRect(hudRect, 8, 8);
    painter.setOpacity(1.0);
    painter.drawText(hudRect.adjusted(12, 12, -12, -12), Qt::AlignLeft | Qt::AlignTop,
                     tr("FPS: %1\nFrame: %2 ms\nDraw Calls: %3")
                         .arg(smoothedFps, 0, 'f', 1)
                         .arg(smoothedFrameMs, 0, 'f', 2)
                         .arg(lastDrawCalls));
}

void GLViewport::drawAxes()
{
    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(0, 0, 0), QVector3D(1, 0, 0) }, QVector4D(1.0f, 0.0f, 0.0f, 1.0f), 2.0f, true, false);
    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(0, 0, 0), QVector3D(0, 1, 0) }, QVector4D(0.0f, 1.0f, 0.0f, 1.0f), 2.0f, true, false);
    renderer.addLineSegments(std::vector<QVector3D>{ QVector3D(0, 0, 0), QVector3D(0, 0, 1) }, QVector4D(0.0f, 0.0f, 1.0f, 1.0f), 2.0f, true, false);
}

void GLViewport::drawGrid()
{
    const int N = 20;
    const float S = 1.0f;
    std::vector<QVector3D> segments;
    segments.reserve(static_cast<size_t>((N * 2 + 1) * 4));
    for (int i = -N; i <= N; ++i) {
        segments.emplace_back(i * S, 0.0f, -N * S);
        segments.emplace_back(i * S, 0.0f, N * S);
        segments.emplace_back(-N * S, 0.0f, i * S);
        segments.emplace_back(N * S, 0.0f, i * S);
    }
    renderer.addLineSegments(segments, QVector4D(0.85f, 0.85f, 0.85f, 1.0f), 1.0f, true, false);
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

void drawCurve(Renderer& renderer,
               const Curve& curve,
               bool selected,
               bool treatAsHidden,
               Renderer::RenderStyle style)
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
        color = QVector4D(0.35f, 0.35f, 0.35f, 0.65f);
        width = 1.8f;
    } else if (style == Renderer::RenderStyle::Monochrome) {
        color = selected ? QVector4D(0.22f, 0.22f, 0.22f, 1.0f) : QVector4D(0.13f, 0.13f, 0.13f, 1.0f);
        width = selected ? 2.6f : 1.8f;
    } else if (style == Renderer::RenderStyle::HiddenLine) {
        color = selected ? QVector4D(0.18f, 0.18f, 0.18f, 1.0f) : QVector4D(0.12f, 0.12f, 0.12f, 1.0f);
        width = selected ? 2.4f : 1.6f;
    } else {
        color = selected ? QVector4D(0.95f, 0.35f, 0.25f, 1.0f) : QVector4D(0.1f, 0.1f, 0.1f, 1.0f);
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
               Renderer::RenderStyle style)
{
    const HalfEdgeMesh& mesh = solid.getMesh();
    const auto& vertices = mesh.getVertices();
    const auto& triangles = mesh.getTriangles();

    QVector4D fillColor = selected ? QVector4D(0.85f, 0.73f, 0.42f, 1.0f) : QVector4D(0.7f, 0.75f, 0.8f, 1.0f);
    if (style == Renderer::RenderStyle::Monochrome) {
        fillColor = selected ? QVector4D(0.93f, 0.93f, 0.93f, 1.0f) : QVector4D(0.82f, 0.84f, 0.86f, 1.0f);
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

    QVector4D edgeColor = selected ? QVector4D(0.35f, 0.15f, 0.05f, 1.0f) : QVector4D(0.1f, 0.1f, 0.1f, 1.0f);
    if (style == Renderer::RenderStyle::Monochrome || style == Renderer::RenderStyle::HiddenLine) {
        edgeColor = selected ? QVector4D(0.2f, 0.2f, 0.2f, 1.0f) : QVector4D(0.13f, 0.13f, 0.13f, 1.0f);
    }
    if (treatAsHidden) {
        edgeColor = QVector4D(0.38f, 0.38f, 0.38f, 0.75f);
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
            QVector4D hiddenEdgeColor = selected ? QVector4D(0.65f, 0.65f, 0.65f, 1.0f)
                                                 : QVector4D(0.55f, 0.55f, 0.55f, 1.0f);
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

void drawGhostCurve(Renderer& renderer, const Curve& curve, const Tool::PreviewGhost& ghost)
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
    renderer.addLineStrip(positions, QVector4D(0.25f, 0.55f, 0.95f, kGhostAlpha), 2.4f, false, true, true);
}

void drawGhostSolid(Renderer& renderer, const Solid& solid, const Tool::PreviewGhost& ghost)
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
        renderer.addLineStrip(positions, QVector4D(0.25f, 0.55f, 0.95f, kGhostAlpha), 2.0f, true, true, true);
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
                      renderStyle);
        } else if (uptr->getType() == ObjectType::Solid) {
            drawSolid(renderer,
                      *static_cast<const Solid*>(uptr.get()),
                      uptr->isSelected(),
                      treatAsHidden,
                      renderStyle);
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
            drawGhostCurve(renderer, *static_cast<const Curve*>(ghost.object), ghost);
        } else if (ghost.object->getType() == ObjectType::Solid) {
            drawGhostSolid(renderer, *static_cast<const Solid*>(ghost.object), ghost);
        }
    }
}

QMatrix4x4 GLViewport::buildProjectionMatrix(float aspect) const
{
    QMatrix4x4 projection;
    const float znear = 0.1f;
    const float zfar = 5000.0f;
    if (aspect <= 0.0f)
        aspect = 1.0f;
    if (camera.getProjectionMode() == CameraController::ProjectionMode::Parallel) {
        float orthoHeight = camera.getOrthoHeight();
        if (orthoHeight <= 0.0f)
            orthoHeight = 1.0f;
        float halfHeight = orthoHeight * 0.5f;
        float halfWidth = halfHeight * aspect;
        projection.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, znear, zfar);
    } else {
        float fov = camera.getFieldOfView();
        if (fov < 1.0f)
            fov = 1.0f;
        projection.perspective(fov, aspect, znear, zfar);
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

void GLViewport::drawAxisGizmo(QPainter& painter, const QMatrix4x4& viewMatrix) const
{
    if (width() <= 0 || height() <= 0)
        return;

    Q_UNUSED(viewMatrix);

    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    QVector3D forward(tx - cx, ty - cy, tz - cz);
    if (forward.lengthSquared() < 1e-6f)
        forward = QVector3D(0.0f, 0.0f, -1.0f);
    forward.normalize();

    QVector3D upVector(0.0f, 1.0f, 0.0f);
    QVector3D right = QVector3D::crossProduct(forward, upVector);
    if (right.lengthSquared() < 1e-6f) {
        upVector = QVector3D(0.0f, 0.0f, 1.0f);
        right = QVector3D::crossProduct(forward, upVector);
    }
    if (right.lengthSquared() < 1e-6f)
        right = QVector3D(1.0f, 0.0f, 0.0f);
    right.normalize();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    struct AxisInfo {
        QVector3D dir;
        QColor color;
        QString label;
    };

    const std::array<AxisInfo, 3> axes = { {
        { QVector3D(1.0f, 0.0f, 0.0f), QColor(215, 63, 63), tr("X") },
        { QVector3D(0.0f, 1.0f, 0.0f), QColor(63, 176, 92), tr("Y") },
        { QVector3D(0.0f, 0.0f, 1.0f), QColor(69, 112, 214), tr("Z") },
    } };

    int frontAxis = 0;
    float bestFront = -1.0f;
    int upAxis = 0;
    float bestUp = -1.0f;

    for (int i = 0; i < axes.size(); ++i) {
        float dotForward = QVector3D::dotProduct(axes[i].dir, forward);
        float absForward = std::fabs(dotForward);
        if (absForward > bestFront) {
            bestFront = absForward;
            frontAxis = i;
        }

        float dotUp = QVector3D::dotProduct(axes[i].dir, up);
        float absUp = std::fabs(dotUp);
        if (absUp > bestUp) {
            bestUp = absUp;
            upAxis = i;
        }
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const float margin = 16.0f;
    const float radius = 34.0f;
    QPointF center(width() - margin - radius, margin + radius);
    QRectF background(center.x() - radius, center.y() - radius, radius * 2.0f, radius * 2.0f);

    painter.setPen(QPen(QColor(24, 26, 32, 200), 1.0f));
    painter.setBrush(QColor(12, 14, 18, 180));
    painter.drawEllipse(background);

    for (int i = 0; i < axes.size(); ++i) {
        const AxisInfo& axis = axes[i];
        float x = QVector3D::dotProduct(axis.dir, right);
        float y = QVector3D::dotProduct(axis.dir, up);
        float z = QVector3D::dotProduct(axis.dir, forward);

        QPointF direction2D(x, -y);
        float length = radius - 10.0f;
        QPointF endPoint = center + direction2D * length;
        QPointF textPoint = center + direction2D * (radius - 2.0f);

        QColor color = axis.color;
        bool highlight = (i == frontAxis || i == upAxis);
        if (highlight)
            color = color.lighter(130);
        int alpha = z >= 0.0f ? 255 : 160;
        if (!highlight)
            alpha = std::min(alpha, 210);
        else
            alpha = 255;
        color.setAlpha(alpha);

        QPen pen(color, highlight ? 3.0f : 2.0f, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(center, endPoint);

        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QRectF(endPoint.x() - 3.5f, endPoint.y() - 3.5f, 7.0f, 7.0f));

        painter.setPen(QPen(Qt::white, 1.0f));
        painter.setFont(QFont(painter.font().family(), 8, QFont::DemiBold));
        QRectF textRect(textPoint.x() - 10.0f, textPoint.y() - 10.0f, 20.0f, 20.0f);
        painter.drawText(textRect, Qt::AlignCenter, axis.label);
    }

    painter.restore();
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
            if (Tool* tool = toolManager->getActiveTool()) {
                tool->handleMouseDown(input);
            }
            update();
            return;
        }
    }

    if (toolManager && e->button() == Qt::LeftButton) {
        if (auto* t = toolManager->getActiveTool()) {
            Tool::PointerInput input;
            input.x = std::lround(devicePos.x());
            input.y = std::lround(devicePos.y());
            input.modifiers = toModifierState(e->modifiers());
            input.devicePixelRatio = ratio;
            toolManager->updatePointerModifiers(input.modifiers);
            t->handleMouseDown(input);
        }
    }
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
                if (Tool* tool = toolManager->getActiveTool()) {
                    tool->handleMouseUp(input);
                }
                if (activeNavigationBinding->temporary && toolManager) {
                    toolManager->restorePreviousTool();
                }
                activeNavigationBinding.reset();
                navigationButton = Qt::NoButton;
            } else {
                if (Tool* tool = toolManager->getActiveTool()) {
                    tool->handleMouseMove(input);
                }
                navigationHandled = true;
            }
        }

        if (!navigationHandled) {
            if (Tool* tool = toolManager->getActiveTool()) {
                tool->handleMouseMove(input);
            }
        }
    }

    QVector3D world;
    if (projectCursorToGround(e->position(), world)) {
        emit cursorPositionChanged(world.x(), world.y(), world.z());
    }

    if (navigationHandled)
        update();
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
            if (Tool* tool = toolManager->getActiveTool()) {
                tool->handleMouseUp(input);
            }
            if (activeNavigationBinding->temporary)
                toolManager->restorePreviousTool();
        }
        activeNavigationBinding.reset();
        navigationButton = Qt::NoButton;
        update();
        return;
    }

    if (toolManager && e->button() == Qt::LeftButton) {
        if (auto* t = toolManager->getActiveTool()) {
            toolManager->updatePointerModifiers(input.modifiers);
            t->handleMouseUp(input);
        }
    }
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
}

void GLViewport::keyPressEvent(QKeyEvent* e)
{
    bool consumed = false;
    if (toolManager) {
        if (Tool* tool = toolManager->getActiveTool()) {
            if (e->key() == Qt::Key_Escape) {
                tool->cancel();
                consumed = true;
            } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                tool->commit();
                consumed = true;
            }
        }
        if (!consumed) {
            toolManager->handleKeyPress(e->key());
        }
    }
    if (!consumed) {
        QOpenGLWidget::keyPressEvent(e);
    }
    update();
}

void GLViewport::keyReleaseEvent(QKeyEvent* e)
{
    if (toolManager) {
        toolManager->handleKeyRelease(e->key());
    }
    QOpenGLWidget::keyReleaseEvent(e);
    update();
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



