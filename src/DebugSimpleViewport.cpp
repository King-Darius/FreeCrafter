#include "DebugSimpleViewport.h"

#include <QOpenGLContext>
#include <QVector3D>
#include <QtMath>
#include <QDebug>

#include <array>

namespace {
struct Vertex {
    QVector3D position;
    QVector3D color;
};

constexpr const char* kVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;
uniform mat4 u_mvp;
out vec3 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

constexpr const char* kFragmentShaderSrc = R"(
#version 330 core
in vec3 v_color;
out vec4 fragColor;
void main() {
    fragColor = vec4(v_color, 1.0);
}
)";
}

DebugSimpleViewport::DebugSimpleViewport(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

DebugSimpleViewport::~DebugSimpleViewport()
{
    makeCurrent();
    vertexBuffer.destroy();
    vao.destroy();
    program.removeAllShaders();
    doneCurrent();
}

void DebugSimpleViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    logContextInformation();

    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShaderSrc)) {
        qWarning() << "DebugSimpleViewport: failed to compile vertex shader" << program.log();
        geometryReady = false;
        return;
    }
    if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShaderSrc)) {
        qWarning() << "DebugSimpleViewport: failed to compile fragment shader" << program.log();
        geometryReady = false;
        return;
    }
    if (!program.link()) {
        qWarning() << "DebugSimpleViewport: failed to link shader program" << program.log();
        geometryReady = false;
        return;
    }

    initializeGeometry();
}

void DebugSimpleViewport::initializeGeometry()
{
    const std::array<Vertex, 3> vertices{ {
        { QVector3D(-0.8f, -0.6f, 0.0f), QVector3D(1.0f, 0.2f, 0.2f) },
        { QVector3D(0.8f, -0.6f, 0.0f), QVector3D(0.2f, 1.0f, 0.2f) },
        { QVector3D(0.0f, 0.9f, 0.0f), QVector3D(0.2f, 0.4f, 1.0f) },
    } };

    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(Vertex)));

    vao.create();
    QOpenGLVertexArrayObject::Binder binder(&vao);
    program.bind();
    program.enableAttributeArray(0);
    program.enableAttributeArray(1);
    program.setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, position), 3, sizeof(Vertex));
    program.setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, color), 3, sizeof(Vertex));
    vertexBuffer.release();

    geometryReady = true;
}

void DebugSimpleViewport::logContextInformation() const
{
    QOpenGLContext* ctx = context();
    if (!ctx || !ctx->isValid()) {
        qWarning() << "DebugSimpleViewport: invalid OpenGL context";
        return;
    }
    const auto fmt = ctx->format();
    // Log only driver metadata; do not include any user-specific paths or identifiers.
    qInfo() << "DebugSimpleViewport: OpenGL context" << fmt.majorVersion() << '.' << fmt.minorVersion()
            << "profile" << fmt.profile();
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    qInfo() << "  GL_VENDOR:" << (vendor ? reinterpret_cast<const char*>(vendor) : "");
    qInfo() << "  GL_RENDERER:" << (renderer ? reinterpret_cast<const char*>(renderer) : "");
    qInfo() << "  GL_VERSION:" << (version ? reinterpret_cast<const char*>(version) : "");
}

void DebugSimpleViewport::resizeGL(int w, int h)
{
    const qreal dpr = devicePixelRatioF();
    const int pixelW = std::max(1, static_cast<int>(std::lround(w * dpr)));
    const int pixelH = std::max(1, static_cast<int>(std::lround(h * dpr)));
    glViewport(0, 0, pixelW, pixelH);

    projection.setToIdentity();
    const float aspect = pixelH > 0 ? static_cast<float>(pixelW) / static_cast<float>(pixelH) : 1.0f;
    projection.perspective(45.0f, aspect, 0.1f, 1000.0f);
}

void DebugSimpleViewport::paintGL()
{
    glClearColor(0.08f, 0.08f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!geometryReady)
        return;

    QMatrix4x4 view;
    view.lookAt(QVector3D(4.5f, 4.5f, 4.5f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));
    const QMatrix4x4 mvp = projection * view;

    program.bind();
    program.setUniformValue("u_mvp", mvp);
    QOpenGLVertexArrayObject::Binder binder(&vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

