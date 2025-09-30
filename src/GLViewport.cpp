#include "GLViewport.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QMatrix4x4>
#include <QVector4D>
#include <QtMath>
#include <limits>

#include <algorithm>
#include <cmath>

#include "Tools/ToolManager.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"
#include "GeometryKernel/Vector3.h"
#include "GeometryKernel/TransformUtils.h"
#include "Interaction/InferenceEngine.h"

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
    renderer.initialize(this);
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
    }
}

void GLViewport::setRenderStyle(Renderer::RenderStyle style)
{
    if (renderStyle == style) {
        return;
    }
    renderStyle = style;
    update();
}

void GLViewport::paintGL()
{
    currentDrawCalls = 0;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float fov = 60.0f;
    float aspect = width() > 0 ? float(width())/float(height()>0?height():1) : 1.0f;
    float znear = 0.1f, zfar = 1000.0f;

    QMatrix4x4 projectionMatrix;
    projectionMatrix.perspective(fov, aspect, znear, zfar);

    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    QVector3D eye(cx, cy, cz);
    QVector3D center(tx, ty, tz);

    QMatrix4x4 viewMatrix;
    viewMatrix.lookAt(eye, center, QVector3D(0.0f, 1.0f, 0.0f));

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
    drawScene();
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

void drawCurve(Renderer& renderer, const Curve& curve, bool selected)
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
    QVector4D color = selected ? QVector4D(0.95f, 0.35f, 0.25f, 1.0f) : QVector4D(0.1f, 0.1f, 0.1f, 1.0f);
    renderer.addLineStrip(positions, color, selected ? 3.0f : 2.0f, false, true, false);
}

void drawSolid(Renderer& renderer, const Solid& solid, bool selected)
{
    const HalfEdgeMesh& mesh = solid.getMesh();
    const auto& vertices = mesh.getVertices();
    const auto& triangles = mesh.getTriangles();

    QVector4D fillColor = selected ? QVector4D(0.85f, 0.73f, 0.42f, 1.0f) : QVector4D(0.7f, 0.75f, 0.8f, 1.0f);
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

    QVector4D edgeColor = selected ? QVector4D(0.35f, 0.15f, 0.05f, 1.0f) : QVector4D(0.1f, 0.1f, 0.1f, 1.0f);
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
        renderer.addLineStrip(positions, edgeColor, selected ? 2.2f : 1.5f, true, true, false, Renderer::LineCategory::Edge);
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

void GLViewport::drawScene()
{
    Tool::PreviewState preview;
    if (toolManager) {
        if (Tool* active = toolManager->getActiveTool()) {
            preview = active->getPreviewState();
        }
    }

    const auto& objs = geometry.getObjects();
    for (const auto& uptr : objs) {
        if (uptr->getType() == ObjectType::Curve) {
            drawCurve(renderer, *static_cast<const Curve*>(uptr.get()), uptr->isSelected());
        } else if (uptr->getType() == ObjectType::Solid) {
            drawSolid(renderer, *static_cast<const Solid*>(uptr.get()), uptr->isSelected());
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
            renderer.addLineStrip(positions, color, 2.0f, poly.closed, true, true);
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

    QMatrix4x4 projection;
    const float aspect = width() > 0 ? static_cast<float>(width()) / static_cast<float>(height() > 0 ? height() : 1) : 1.0f;
    projection.perspective(60.0f, aspect, 0.1f, 1000.0f);

    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    QMatrix4x4 view;
    view.lookAt(QVector3D(cx, cy, cz), QVector3D(tx, ty, tz), QVector3D(0.0f, 1.0f, 0.0f));

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

void GLViewport::mousePressEvent(QMouseEvent* e)
{
    if (toolManager) {
        toolManager->updatePointerModifiers(toModifierState(e->modifiers()));
    }
    lastMouse = e->pos();
    if (e->buttons() & Qt::RightButton) {
        rotating = true;
    } else if (e->buttons() & Qt::MiddleButton) {
        panning = true;
    }
    if (toolManager && e->button() == Qt::LeftButton) {
        if (auto* t = toolManager->getActiveTool()) {
            const auto ratio = devicePixelRatioF();
            const QPointF devicePos = e->position() * ratio;
            lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
            hasDeviceMouse = true;
            Tool::PointerInput input;
            input.x = std::lround(devicePos.x());
            input.y = std::lround(devicePos.y());
            input.modifiers = toModifierState(e->modifiers());
            toolManager->updatePointerModifiers(input.modifiers);
            t->handleMouseDown(input);
        }
    }
}

void GLViewport::mouseMoveEvent(QMouseEvent* e)
{
    QPoint delta = e->pos() - lastMouse;
    lastMouse = e->pos();

    const auto ratio = devicePixelRatioF();
    const QPointF devicePos = e->position() * ratio;
    lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
    hasDeviceMouse = true;

    if (rotating) {
        camera.rotateCamera(delta.x(), delta.y());
        update(); return;
    }
    if (panning) {
        camera.panCamera(delta.x(), delta.y());
        update(); return;
    }

    if (toolManager) {
        Tool::PointerInput input;
        input.x = std::lround(devicePos.x());
        input.y = std::lround(devicePos.y());
        input.modifiers = toModifierState(e->modifiers());
        toolManager->updatePointerModifiers(input.modifiers);
        if (auto* t = toolManager->getActiveTool()) {
            t->handleMouseMove(input);
        }
    }

    QVector3D world;
    if (projectCursorToGround(e->position(), world)) {
        emit cursorPositionChanged(world.x(), world.y(), world.z());
    }
}

void GLViewport::mouseReleaseEvent(QMouseEvent* e)
{
    if (toolManager) {
        toolManager->updatePointerModifiers(toModifierState(e->modifiers()));
    }
    if (rotating && !(e->buttons() & Qt::RightButton)) rotating = false;
    if (panning && !(e->buttons() & Qt::MiddleButton)) panning = false;

    if (toolManager && e->button() == Qt::LeftButton) {
        if (auto* t = toolManager->getActiveTool()) {
            const auto ratio = devicePixelRatioF();
            const QPointF devicePos = e->position() * ratio;
            lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
            hasDeviceMouse = true;
            Tool::PointerInput input;
            input.x = std::lround(devicePos.x());
            input.y = std::lround(devicePos.y());
            input.modifiers = toModifierState(e->modifiers());
            toolManager->updatePointerModifiers(input.modifiers);
            t->handleMouseUp(input);
        }
    }
}

void GLViewport::wheelEvent(QWheelEvent* e)
{
    camera.zoomCamera(e->angleDelta().y()/120.0f);
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

    const float fov = 60.0f;
    const float aspect = width() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    const float znear = 0.1f;
    const float zfar = 1000.0f;

    QMatrix4x4 projection;
    projection.perspective(fov, aspect, znear, zfar);

    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    QMatrix4x4 view;
    view.lookAt(QVector3D(cx, cy, cz), QVector3D(tx, ty, tz), QVector3D(0.0f, 1.0f, 0.0f));

    bool invertible = false;
    const QMatrix4x4 inv = (projection * view).inverted(&invertible);
    if (!invertible)
        return false;

    const float nx = (2.0f * pos.x()) / static_cast<float>(width()) - 1.0f;
    const float ny = 1.0f - (2.0f * pos.y()) / static_cast<float>(height());

    const QVector4D nearPoint(nx, ny, -1.0f, 1.0f);
    const QVector4D farPoint(nx, ny, 1.0f, 1.0f);

    QVector4D worldNear = inv * nearPoint;
    QVector4D worldFar = inv * farPoint;
    if (qFuzzyIsNull(worldNear.w()) || qFuzzyIsNull(worldFar.w()))
        return false;
    worldNear /= worldNear.w();
    worldFar /= worldFar.w();

    QVector3D origin = worldNear.toVector3D();
    QVector3D direction = (worldFar - worldNear).toVector3D();
    if (qFuzzyIsNull(direction.lengthSquared()))
        return false;
    direction.normalize();

    if (qFuzzyIsNull(direction.y()))
        return false;
    const float t = -origin.y() / direction.y();
    if (t < 0.0f)
        return false;

    world = origin + direction * t;
    return true;
}
