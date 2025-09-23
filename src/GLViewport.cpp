#include "GLViewport.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

#include <algorithm>
#include <cmath>

#include "Tools/ToolManager.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"

GLViewport::GLViewport(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // small timer to keep UI responsive during drags
    repaintTimer.setInterval(16);
    connect(&repaintTimer, &QTimer::timeout, this, QOverload<>::of(&GLViewport::update));
    repaintTimer.start();
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

    drawGrid();
    drawAxes();
    drawScene();
}

void GLViewport::drawAxes()
{
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(1,0,0); // X
    glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,1,0); // Y
    glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,1); // Z
    glEnd();
}

void GLViewport::drawGrid()
{
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
            glBegin(GL_LINE_STRIP);
            for (const auto& p : pts) glVertex3f(p.x, p.y, p.z);
            glEnd();
        } else if (uptr->getType() == ObjectType::Solid) {
            const Solid* s = static_cast<const Solid*>(uptr.get());
            const auto& verts = s->getVertices();
            const auto& faces = s->getFaces();
            glColor3f(0.7f,0.75f,0.8f);
            glBegin(GL_QUADS);
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
                glBegin(GL_LINE_LOOP);
                for (int idx : face.indices) {
                    const auto& v = verts[(size_t)idx];
                    glVertex3f(v.x, v.y, v.z);
                }
                glEnd();
            }
            size_t ringSize = verts.size() / 2;
            if (ringSize >= 2) {
                glBegin(GL_LINE_LOOP);
                for (size_t i=0;i<ringSize;++i) {
                    const auto& v = verts[i];
                    glVertex3f(v.x, v.y, v.z);
                }
                glEnd();
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
            t->onMouseDown(std::lround(devicePos.x()), std::lround(devicePos.y()));
        }
    }
}

void GLViewport::mouseMoveEvent(QMouseEvent* e)
{
    QPoint delta = e->pos() - lastMouse;
    lastMouse = e->pos();

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
            const auto ratio = devicePixelRatioF();
            const QPointF devicePos = e->position() * ratio;
            t->onMouseMove(std::lround(devicePos.x()), std::lround(devicePos.y()));
        }
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
        if (auto* t = toolManager->getActiveTool()) {
            t->onKeyPress(static_cast<char>(e->key()));
        }
    }
}
