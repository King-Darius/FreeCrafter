#include "Renderer.h"

#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QtMath>
#include <utility>

namespace {
const char* kLineVertexShader = R"(\
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
uniform mat4 u_mvp;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

const char* kLineFragmentShader = R"(\
#version 330 core
in vec4 v_color;
out vec4 fragColor;
void main() {
    fragColor = v_color;
}
)";

const char* kTriangleVertexShader = R"(\
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform mat3 u_normalMatrix;
uniform vec3 u_lightDir;
out vec3 v_normal;
out vec4 v_color;
out float v_lighting;
void main() {
    vec3 normal = normalize(u_normalMatrix * a_normal);
    v_normal = normal;
    v_color = a_color;
    float diffuse = max(dot(normal, normalize(u_lightDir)), 0.0);
    v_lighting = diffuse;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

const char* kTriangleFragmentShader = R"(\
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in float v_lighting;
out vec4 fragColor;
void main() {
    float ambient = 0.25;
    float lighting = ambient + (1.0 - ambient) * clamp(v_lighting, 0.0, 1.0);
    vec3 color = v_color.rgb * lighting;
    fragColor = vec4(color, v_color.a);
}
)";
}

Renderer::Renderer()
    : lineBuffer(QOpenGLBuffer::VertexBuffer)
    , triangleBuffer(QOpenGLBuffer::VertexBuffer)
{
}

void Renderer::initialize(QOpenGLFunctions* funcs)
{
    functions = funcs;
    if (!lineBuffer.isCreated()) {
        lineBuffer.create();
    }
    if (!triangleBuffer.isCreated()) {
        triangleBuffer.create();
    }
    if (!lineVao.isCreated()) {
        lineVao.create();
    }
    if (!triangleVao.isCreated()) {
        triangleVao.create();
    }
    programsReady = false;
}

void Renderer::beginFrame(const QMatrix4x4& projection, const QMatrix4x4& view, RenderStyle style)
{
    ensurePrograms();
    lineBatches.clear();
    triangleVertices.clear();
    currentStyle = style;
    mvp = projection * view;
    normalMatrix = view.normalMatrix();
    QVector3D lightWorld(0.3f, 0.8f, 0.6f);
    QVector3D transformed = view.mapVector(lightWorld);
    if (!transformed.isNull()) {
        lightDir = transformed.normalized();
    } else {
        lightDir = QVector3D(0.0f, 1.0f, 0.0f);
    }
}

Renderer::LineBatch& Renderer::fetchBatch(float width, bool depthTest, bool blend, LineCategory category)
{
    for (auto& batch : lineBatches) {
        if (qFuzzyCompare(batch.config.width, width)
            && batch.config.depthTest == depthTest
            && batch.config.blend == blend
            && batch.config.category == category) {
            return batch;
        }
    }
    LineBatch batch;
    batch.config.width = width;
    batch.config.depthTest = depthTest;
    batch.config.blend = blend;
    batch.config.category = category;
    lineBatches.push_back(std::move(batch));
    return lineBatches.back();
}

void Renderer::addLineSegments(const std::vector<QVector3D>& segments,
                               const QVector4D& color,
                               float width,
                               bool depthTest,
                               bool blend,
                               LineCategory category)
{
    if (segments.size() < 2) {
        return;
    }
    auto& batch = fetchBatch(width, depthTest, blend, category);
    batch.vertices.reserve(batch.vertices.size() + segments.size());
    for (size_t i = 1; i < segments.size(); i += 2) {
        const QVector3D& a = segments[i - 1];
        const QVector3D& b = segments[i];
        batch.vertices.push_back({ a, color });
        batch.vertices.push_back({ b, color });
    }
}

void Renderer::addLineStrip(const std::vector<QVector3D>& points,
                            const QVector4D& color,
                            float width,
                            bool closed,
                            bool depthTest,
                            bool blend,
                            LineCategory category)
{
    if (points.size() < 2) {
        return;
    }
    auto& batch = fetchBatch(width, depthTest, blend, category);
    batch.vertices.reserve(batch.vertices.size() + (points.size() - 1 + (closed ? 1 : 0)) * 2);
    for (size_t i = 1; i < points.size(); ++i) {
        batch.vertices.push_back({ points[i - 1], color });
        batch.vertices.push_back({ points[i], color });
    }
    if (closed) {
        batch.vertices.push_back({ points.back(), color });
        batch.vertices.push_back({ points.front(), color });
    }
}

void Renderer::addTriangle(const QVector3D& a,
                           const QVector3D& b,
                           const QVector3D& c,
                           const QVector3D& normal,
                           const QVector4D& color)
{
    triangleVertices.push_back({ a, normal, color });
    triangleVertices.push_back({ b, normal, color });
    triangleVertices.push_back({ c, normal, color });
}

void Renderer::ensurePrograms()
{
    if (programsReady) {
        return;
    }

    if (!lineProgram.isLinked()) {
        lineProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, kLineVertexShader);
        lineProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kLineFragmentShader);
        lineProgram.link();
    }

    if (!triangleProgram.isLinked()) {
        triangleProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, kTriangleVertexShader);
        triangleProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kTriangleFragmentShader);
        triangleProgram.link();
    }

    programsReady = lineProgram.isLinked() && triangleProgram.isLinked();
}

void Renderer::ensureLineState(const LineBatch& batch)
{
    QOpenGLVertexArrayObject::Binder binder(&lineVao);
    lineBuffer.bind();
    lineBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    lineBuffer.allocate(batch.vertices.data(), static_cast<int>(batch.vertices.size() * sizeof(LineVertex)));

    lineProgram.bind();
    lineProgram.setUniformValue("u_mvp", mvp);
    lineProgram.enableAttributeArray(0);
    lineProgram.setAttributeBuffer(0, GL_FLOAT, offsetof(LineVertex, position), 3, sizeof(LineVertex));
    lineProgram.enableAttributeArray(1);
    lineProgram.setAttributeBuffer(1, GL_FLOAT, offsetof(LineVertex, color), 4, sizeof(LineVertex));

    if (batch.config.depthTest) {
        functions->glEnable(GL_DEPTH_TEST);
    } else {
        functions->glDisable(GL_DEPTH_TEST);
    }
    if (batch.config.blend) {
        functions->glEnable(GL_BLEND);
        functions->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        functions->glDisable(GL_BLEND);
    }
    functions->glLineWidth(batch.config.width);
}

void Renderer::ensureTriangleState()
{
    QOpenGLVertexArrayObject::Binder binder(&triangleVao);
    triangleBuffer.bind();
    triangleBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    triangleBuffer.allocate(triangleVertices.data(), static_cast<int>(triangleVertices.size() * sizeof(TriangleVertex)));

    triangleProgram.bind();
    triangleProgram.setUniformValue("u_mvp", mvp);
    triangleProgram.setUniformValue("u_normalMatrix", normalMatrix);
    triangleProgram.setUniformValue("u_lightDir", lightDir);
    triangleProgram.enableAttributeArray(0);
    triangleProgram.setAttributeBuffer(0, GL_FLOAT, offsetof(TriangleVertex, position), 3, sizeof(TriangleVertex));
    triangleProgram.enableAttributeArray(1);
    triangleProgram.setAttributeBuffer(1, GL_FLOAT, offsetof(TriangleVertex, normal), 3, sizeof(TriangleVertex));
    triangleProgram.enableAttributeArray(2);
    triangleProgram.setAttributeBuffer(2, GL_FLOAT, offsetof(TriangleVertex, color), 4, sizeof(TriangleVertex));

    functions->glDisable(GL_BLEND);
    functions->glEnable(GL_DEPTH_TEST);
    functions->glLineWidth(1.0f);
}

int Renderer::flush()
{
    ensurePrograms();
    int draws = 0;

    if (currentStyle != RenderStyle::Wireframe && !triangleVertices.empty()) {
        ensureTriangleState();
        functions->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangleVertices.size()));
        ++draws;
    }

    for (const auto& batch : lineBatches) {
        if (batch.vertices.empty()) {
            continue;
        }
        if (batch.config.category == LineCategory::Edge && currentStyle == RenderStyle::Shaded) {
            continue;
        }
        ensureLineState(batch);
        functions->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(batch.vertices.size()));
        ++draws;
    }

    functions->glDisable(GL_BLEND);
    functions->glEnable(GL_DEPTH_TEST);
    functions->glLineWidth(1.0f);

    return draws;
}

