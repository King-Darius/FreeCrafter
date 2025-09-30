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

void GLViewport::paintGL()
{
    currentDrawCalls = 0;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // very small fixed function style camera (compat profile)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float fov = 60.0f;
    float aspect = width() > 0 ? float(width())/float(height()>0?height():1) : 1.0f;
    float znear = 0.1f, zfar = 1000.0f;
    float top = znear * tanf(fov * float(M_PI)/360.0f);
    float right = top * aspect;
    glFrustum(-right, right, -top, top, znear, zfar);

    QMatrix4x4 projectionMatrix;
    projectionMatrix.perspective(fov, aspect, znear, zfar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float cx, cy, cz;
    camera.getCameraPosition(cx, cy, cz);
    float tx, ty, tz;
    camera.getTarget(tx, ty, tz);

    // Simple lookAt
    QVector3D eye(cx, cy, cz), center(tx, ty, tz), up(0,1,0);
    QVector3D f = (center-eye); f.normalize();
    QVector3D s = QVector3D::crossProduct(f, up); s.normalize();
    QVector3D u = QVector3D::crossProduct(s, f);

    QMatrix4x4 viewMatrix;
    viewMatrix.lookAt(eye, center, QVector3D(0.0f, 1.0f, 0.0f));

    float M[16] = {
        s.x(),  u.x(),  -f.x(),  0,
        s.y(),  u.y(),  -f.y(),  0,
        s.z(),  u.z(),  -f.z(),  0,
        -QVector3D::dotProduct(s, eye),
        -QVector3D::dotProduct(u, eye),
        QVector3D::dotProduct(f, eye),
        1
    };
    glMultMatrixf(M);

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

    drawGrid();
    drawAxes();
    drawScene();

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
    ++currentDrawCalls;
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(1,0,0); // X
    glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,1,0); // Y
    glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,1); // Z
    glEnd();
}

void GLViewport::drawGrid()
{
    ++currentDrawCalls;
    glLineWidth(1.0f);
    glColor3f(0.85f,0.85f,0.85f);
    const int N = 20;
    const float S = 1.0f;
    glBegin(GL_LINES);
    for (int i=-N;i<=N;++i) {
        glVertex3f(i*S, 0, -N*S);
        glVertex3f(i*S, 0,  N*S);
        glVertex3f(-N*S, 0, i*S);
        glVertex3f( N*S, 0, i*S);
    }
    glEnd();
}

void GLViewport::drawScene()
{
    const auto& objs = geometry.getObjects();
    for (const auto& uptr : objs) {
        if (uptr->getType() == ObjectType::Curve) {
            const Curve* c = static_cast<const Curve*>(uptr.get());
            const auto& pts = c->getPoints();
            if (pts.size() < 2) continue;
            glColor3f(0.1f, 0.1f, 0.1f);
            glLineWidth(2.0f);
            ++currentDrawCalls;
            glBegin(GL_LINE_STRIP);
            for (const auto& p : pts) glVertex3f(p.x, p.y, p.z);
            glEnd();
        } else if (uptr->getType() == ObjectType::Solid) {
            const Solid* s = static_cast<const Solid*>(uptr.get());
            const auto& verts = s->getVertices();
            const auto& faces = s->getFaces();
            glColor3f(0.7f,0.75f,0.8f);
            glBegin(GL_QUADS);
            ++currentDrawCalls;
            for (const auto& f : faces) {
                if (f.indices.size() == 4) {
                    for (int idx : f.indices) {
                        const auto& v = verts[(size_t)idx];
                        glVertex3f(v.x, v.y, v.z);
                    }
                }
            }
            glEnd();
            // draw edges
            glColor3f(0.1f,0.1f,0.1f);
            glLineWidth(1.5f);
            for (const auto& face : faces) {
                if (face.indices.size() < 2) continue;
                ++currentDrawCalls;
                glBegin(GL_LINE_LOOP);
                for (int idx : face.indices) {
                    const auto& v = verts[(size_t)idx];
                    glVertex3f(v.x, v.y, v.z);
                }
                glEnd();
            }
            size_t ringSize = verts.size() / 2;
            if (ringSize >= 2) {
                ++currentDrawCalls;
                glBegin(GL_LINE_LOOP);
                for (size_t i=0;i<ringSize;++i) {
                    const auto& v = verts[i];
                    glVertex3f(v.x, v.y, v.z);
                }
                glEnd();
                ++currentDrawCalls;
                glBegin(GL_LINE_LOOP);
                for (size_t i=0;i<ringSize;++i) {
                    const auto& v = verts[i + ringSize];
                    glVertex3f(v.x, v.y, v.z);
                }
                glEnd();
            }
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
            t->onMouseDown(std::lround(devicePos.x()), std::lround(devicePos.y()));
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
        if (auto* t = toolManager->getActiveTool()) {
            t->onMouseMove(std::lround(devicePos.x()), std::lround(devicePos.y()));
        }
    }

    QVector3D world;
    if (projectCursorToGround(e->position(), world)) {
        emit cursorPositionChanged(world.x(), world.y(), world.z());
    }
}

void GLViewport::mouseReleaseEvent(QMouseEvent* e)
{
    if (rotating && !(e->buttons() & Qt::RightButton)) rotating = false;
    if (panning && !(e->buttons() & Qt::MiddleButton)) panning = false;

    if (toolManager && e->button() == Qt::LeftButton) {
        if (auto* t = toolManager->getActiveTool()) {
            const auto ratio = devicePixelRatioF();
            const QPointF devicePos = e->position() * ratio;
            lastDeviceMouse = QPoint(std::lround(devicePos.x()), std::lround(devicePos.y()));
            hasDeviceMouse = true;
            t->onMouseUp(std::lround(devicePos.x()), std::lround(devicePos.y()));
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
    if (toolManager) {
        toolManager->handleKeyPress(e->key());
    }
    QOpenGLWidget::keyPressEvent(e);
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
