#include "Renderer.h"

#include <QOpenGLExtraFunctions>
const char* kLineFragmentShader = R"(
#version 330 core
in vec4 v_color;
uniform float u_stippleEnabled;
uniform float u_stippleScale;
uniform float u_profileMix;
uniform vec4 u_profileColor;
out vec4 fragColor;
void main() {
    if (u_stippleEnabled > 0.5) {
        float scale = max(u_stippleScale, 1.0);
        float pattern = mod(gl_FragCoord.x + gl_FragCoord.y, scale);
        if (pattern < 0.5 * scale) {
            discard;
        }
    }
    vec4 baseColor = mix(v_color, u_profileColor, clamp(u_profileMix, 0.0, 1.0));
    fragColor = baseColor;
}
)";

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
uniform mat4 u_mvp;
uniform int u_clipPlaneCount;
uniform vec4 u_clipPlanes[4];
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
    for (int i = 0; i < 4; ++i) {
        if (i < u_clipPlaneCount) {
            gl_ClipDistance[i] = dot(vec4(a_position, 1.0), u_clipPlanes[i]);
        } else {
            gl_ClipDistance[i] = 1.0;
        }
    }
}
)";

const char* kLineFragmentShader = R"(
#version 330 core
in vec4 v_color;
uniform float u_stippleEnabled;
uniform float u_stippleScale;
out vec4 fragColor;
void main() {
    if (u_stippleEnabled > 0.5) {
        float scale = max(u_stippleScale, 1.0);
        float pattern = mod(gl_FragCoord.x + gl_FragCoord.y, scale);
        if (pattern < 0.5 * scale) {
            discard;
        }
    }
    fragColor = v_color;
}
)";

const char* kTriangleVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform mat4 u_lightMVP;
uniform mat3 u_normalMatrix;
uniform vec3 u_lightDir;
uniform int u_clipPlaneCount;
uniform vec4 u_clipPlanes[4];
out vec3 v_normal;
out vec4 v_color;
out float v_lighting;
out vec4 v_shadowCoord;
void main() {
    vec4 worldPosition = vec4(a_position, 1.0);
    vec3 normal = normalize(u_normalMatrix * a_normal);
    v_normal = normal;
    v_color = a_color;
    float diffuse = max(dot(normal, normalize(u_lightDir)), 0.0);
    v_lighting = diffuse;
    v_shadowCoord = u_lightMVP * worldPosition;
    gl_Position = u_mvp * worldPosition;
    for (int i = 0; i < 4; ++i) {
        if (i < u_clipPlaneCount) {
            gl_ClipDistance[i] = dot(worldPosition, u_clipPlanes[i]);
        } else {
            gl_ClipDistance[i] = 1.0;
        }
    }
}
)";

const char* kTriangleFragmentShader = R"(
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in float v_lighting;
in vec4 v_shadowCoord;
uniform int u_styleMode;
uniform sampler2D u_shadowMap;
uniform float u_shadowEnabled;
uniform float u_shadowStrength;
uniform float u_shadowBias;
uniform vec2 u_shadowTexelSize;
uniform int u_shadowSampleRadius;
out vec4 fragColor;
float computeShadow(vec4 shadowCoord) {
    vec3 projCoords = shadowCoord.xyz / max(shadowCoord.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0)
        return 1.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;
    int radius = max(u_shadowSampleRadius, 0);
    float occlusion = 0.0;
    int samples = 0;
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            vec2 offset = vec2(x, y) * u_shadowTexelSize;
            float depth = texture(u_shadowMap, projCoords.xy + offset).r;
            occlusion += projCoords.z - u_shadowBias > depth ? 1.0 : 0.0;
            ++samples;
        }
    }
    float visibility = 1.0;
    if (samples > 0)
        visibility = 1.0 - clamp(occlusion / float(samples), 0.0, 1.0) * clamp(u_shadowStrength, 0.0, 1.0);
    return clamp(visibility, 0.0, 1.0);
}
void main() {
    if (u_styleMode == 1) {
        vec3 base = mix(vec3(0.82), v_color.rgb, 0.35);
        fragColor = vec4(base, v_color.a);
        return;
    }
    float ambient = 0.25;
    float lighting = ambient + (1.0 - ambient) * clamp(v_lighting, 0.0, 1.0);
    float shadowFactor = 1.0;
    if (u_shadowEnabled > 0.5) {
        shadowFactor = computeShadow(v_shadowCoord);
    }
    vec3 color = v_color.rgb * lighting * shadowFactor;
    fragColor = vec4(color, v_color.a);
}
)";

const char* kShadowVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_lightMVP;
uniform int u_clipPlaneCount;
uniform vec4 u_clipPlanes[4];
void main() {
    vec4 worldPosition = vec4(a_position, 1.0);
    gl_Position = u_lightMVP * worldPosition;
    for (int i = 0; i < 4; ++i) {
        if (i < u_clipPlaneCount) {
            gl_ClipDistance[i] = dot(worldPosition, u_clipPlanes[i]);
        } else {
            gl_ClipDistance[i] = 1.0;
        }
    }
}
)";

const char* kShadowFragmentShader = R"(
#version 330 core
void main() {
}
)";
}

Renderer::Renderer()
    : lineBuffer(QOpenGLBuffer::VertexBuffer)
    , triangleBuffer(QOpenGLBuffer::VertexBuffer)
{
}

Renderer::~Renderer()
{
    releaseShadowResources();
}

void Renderer::initialize(QOpenGLExtraFunctions* funcs)
{
    functions = funcs;
    if (!lineBuffer.isCreated())    lineProgram.setUniformValue("u_mvp", mvp);
    lineProgram.setUniformValue("u_clipPlaneCount", clipPlaneCount);
    if (clipPlaneCount > 0)
        lineProgram.setUniformValueArray("u_clipPlanes", clipPlanes.data(), clipPlaneCount);
    lineProgram.setUniformValue("u_stippleEnabled", batch.config.stippled ? 1.0f : 0.0f);
    lineProgram.setUniformValue("u_stippleScale", batch.config.stippleScale);
    lineProgram.setUniformValue("u_profileMix", 0.0f);
    lineProgram.setUniformValue("u_profileColor", QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
    lineProgram.enableAttributeArray(0);
    lineProgram.setAttributeBuffer(0, GL_FLOAT, offsetof(LineVertex, position), 3, sizeof(LineVertex));
    lineProgram.enableAttributeArray(1);
    lineProgram.setAttributeBuffer(1, GL_FLOAT, offsetof(LineVertex, color), 4, sizeof(LineVertex));

    if (!triangleVao.isCreated())
        triangleVao.create();
    if (!shadowVao.isCreated())
        shadowVao.create();
    programsReady = false;
    shadowMapReady = false;
}

void Renderer::setLightingOptions(const LightingOptions& options)
{
    lightingOptions = options;
    lightingOptions.shadowStrength = std::clamp(lightingOptions.shadowStrength, 0.0f, 1.0f);
    lightingOptions.shadowBias = std::max(lightingOptions.shadowBias, 0.00005f);
    lightingOptions.shadowSampleRadius = std::max(0, lightingOptions.shadowSampleRadius);
}

void Renderer::beginFrame(const QMatrix4x4& projection, const QMatrix4x4& view, RenderStyle style)
{
    ensurePrograms();
    lineBatches.clear();
    triangleVertices.clear();
    currentStyle = style;
    mvp = projection * view;
    normalMatrix = view.normalMatrix();

    QVector3D worldDir = lightingOptions.sunValid ? lightingOptions.sunDirection : QVector3D(0.3f, 0.8f, 0.6f);
    if (worldDir.isNull())
        worldDir = QVector3D(0.3f, 0.8f, 0.6f);
    QVector3D transformed = view.mapVector(worldDir);
    if (!transformed.isNull())
        lightDir = transformed.normalized();
    else
        lightDir = QVector3D(0.0f, 1.0f, 0.0f);

    clipPlaneCount = 0;
    boundsValid = false;
    triangleBufferDirty = true;
    shadowMapReady = false;
}

void Renderer::setClipPlanes(const std::vector<QVector4D>& planes)
{
    clipPlaneCount = std::min<int>(planes.size(), static_cast<int>(clipPlanes.size()));
    for (int i = 0; i < clipPlaneCount; ++i)
        clipPlanes[static_cast<size_t>(i)] = planes[static_cast<size_t>(i)];
}

Renderer::LineBatch& Renderer::fetchBatch(float width,
                                          bool depthTest,
                                          bool blend,
                                          LineCategory category,
                                          bool stippled,
                                          float stippleScale)
{
    for (auto& batch : lineBatches) {
        if (qFuzzyCompare(batch.config.width, width)
            && batch.config.depthTest == depthTest
            && batch.config.blend == blend
            && batch.config.category == category
            && batch.config.stippled == stippled
            && qFuzzyCompare(batch.config.stippleScale, stippleScale)) {
            return batch;
        }
    }
    LineBatch batch;
    batch.config.width = width;
    batch.config.depthTest = depthTest;
    batch.config.blend = blend;
    batch.config.category = category;
    batch.config.stippled = stippled;
    batch.config.stippleScale = stippleScale;
    lineBatches.push_back(std::move(batch));
    return lineBatches.back();
}

void Renderer::addLineSegments(const std::vector<QVector3D>& segments,
                               const QVector4D& color,
                               float width,
                               bool depthTest,
                               bool blend,
                               LineCategory category,
                               bool stippled,
                               float stippleScale)
{
    if (segments.size() < 2)
        return;
    auto& batch = fetchBatch(width, depthTest, blend, category, stippled, stippleScale);
    batch.vertices.reserve(batch.vertices.size() + segments.size());
    for (size_t i = 1; i < segments.size(); i += 2) {
        batch.vertices.push_back({ segments[i - 1], color });
        batch.vertices.push_back({ segments[i], color });
    }
}

void Renderer::addLineStrip(const std::vector<QVector3D>& points,
                            const QVector4D& color,
                            float width,
                            bool closed,
                            bool depthTest,
                            bool blend,
                            LineCategory category,
                            bool stippled,
                            float stippleScale)
{
    if (points.size() < 2)
        return;
    auto& batch = fetchBatch(width, depthTest, blend, category, stippled, stippleScale);
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

    auto updateBounds = [this](const QVector3D& p) {
        if (!boundsValid) {
            boundsMin = boundsMax = p;
            boundsValid = true;
        } else {
            boundsMin.setX(std::min(boundsMin.x(), p.x()));
            boundsMin.setY(std::min(boundsMin.y(), p.y()));
            boundsMin.setZ(std::min(boundsMin.z(), p.z()));
            boundsMax.setX(std::max(boundsMax.x(), p.x()));
            boundsMax.setY(std::max(boundsMax.y(), p.y()));
            boundsMax.setZ(std::max(boundsMax.z(), p.z()));
        }
    };
    updateBounds(a);    bool profilePassActive = currentStyle == RenderStyle::Shaded
        || currentStyle == RenderStyle::ShadedWithEdges
        || currentStyle == RenderStyle::Monochrome;

    if (profilePassActive) {
        for (const auto& batch : lineBatches) {
            if (batch.vertices.empty())
                continue;
            if (batch.config.category != LineCategory::Edge)
                continue;
            if (!batch.config.depthTest)
                continue;
            ensureLineState(batch);
            lineProgram.setUniformValue("u_profileColor", QVector4D(0.05f, 0.05f, 0.05f, 1.0f));
            lineProgram.setUniformValue("u_profileMix", 1.0f);
            functions->glLineWidth(batch.config.width + 1.2f);
            functions->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(batch.vertices.size()));
            ++draws;
            lineProgram.setUniformValue("u_profileMix", 0.0f);
            functions->glLineWidth(batch.config.width);
        }
    }

    for (const auto& batch : lineBatches) {
        if (batch.vertices.empty())
            continue;
        if (batch.config.category == LineCategory::Edge && currentStyle == RenderStyle::Shaded)
            continue;
        ensureLineState(batch);

}

void Renderer::ensurePrograms()
{
    if (programsReady)
        return;

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

    if (!shadowProgram.isLinked()) {
        shadowProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, kShadowVertexShader);
        shadowProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kShadowFragmentShader);
        shadowProgram.link();
    }

    programsReady = lineProgram.isLinked() && triangleProgram.isLinked() && shadowProgram.isLinked();
}

void Renderer::uploadTriangleBufferIfNeeded()
{
    if (!triangleBufferDirty || triangleVertices.empty())
        return;

    if (!triangleBuffer.bind())
        return;

    triangleBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    triangleBuffer.allocate(triangleVertices.data(),
                             static_cast<int>(triangleVertices.size() * sizeof(TriangleVertex)));
    triangleBuffer.release();
    triangleBufferDirty = false;
}


void Renderer::ensureLineState(const LineBatch& batch)
{
    QOpenGLVertexArrayObject::Binder binder(&lineVao);
    lineBuffer.bind();
    lineBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    lineBuffer.allocate(batch.vertices.data(), static_cast<int>(batch.vertices.size() * sizeof(LineVertex)));

    lineProgram.bind();
    lineProgram.setUniformValue("u_mvp", mvp);
    lineProgram.setUniformValue("u_clipPlaneCount", clipPlaneCount);
    if (clipPlaneCount > 0)
        lineProgram.setUniformValueArray("u_clipPlanes", clipPlanes.data(), clipPlaneCount);
    lineProgram.setUniformValue("u_stippleEnabled", batch.config.stippled ? 1.0f : 0.0f);
    lineProgram.setUniformValue("u_stippleScale", batch.config.stippleScale);
    lineProgram.enableAttributeArray(0);
    lineProgram.setAttributeBuffer(0, GL_FLOAT, offsetof(LineVertex, position), 3, sizeof(LineVertex));
    lineProgram.enableAttributeArray(1);
    lineProgram.setAttributeBuffer(1, GL_FLOAT, offsetof(LineVertex, color), 4, sizeof(LineVertex));

    if (functions) {
        for (int i = 0; i < 4; ++i) {
            if (i < clipPlaneCount)
                functions->glEnable(GL_CLIP_DISTANCE0 + i);
            else
                functions->glDisable(GL_CLIP_DISTANCE0 + i);
        }
    }

    if (batch.config.depthTest)
        functions->glEnable(GL_DEPTH_TEST);
    else
        functions->glDisable(GL_DEPTH_TEST);

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
    if (triangleVertices.empty())
        return;

    uploadTriangleBufferIfNeeded();

    QOpenGLVertexArrayObject::Binder binder(&triangleVao);
    triangleBuffer.bind();

    triangleProgram.bind();
    triangleProgram.setUniformValue("u_mvp", mvp);
    triangleProgram.setUniformValue("u_normalMatrix", normalMatrix);
    triangleProgram.setUniformValue("u_lightDir", lightDir);
    triangleProgram.setUniformValue("u_lightMVP", lightViewProjection);
    triangleProgram.setUniformValue("u_clipPlaneCount", clipPlaneCount);
    if (clipPlaneCount > 0)
        triangleProgram.setUniformValueArray("u_clipPlanes", clipPlanes.data(), clipPlaneCount);

    int styleMode = currentStyle == RenderStyle::Monochrome ? 1 : 0;
    triangleProgram.setUniformValue("u_styleMode", styleMode);

    const bool enableShadows = shadowMapReady && lightingOptions.shadowsEnabled && lightingOptions.sunValid;
    triangleProgram.setUniformValue("u_shadowEnabled", enableShadows ? 1.0f : 0.0f);
    triangleProgram.setUniformValue("u_shadowStrength", lightingOptions.shadowStrength);
    triangleProgram.setUniformValue("u_shadowBias", lightingOptions.shadowBias);
    triangleProgram.setUniformValue("u_shadowSampleRadius", lightingOptions.shadowSampleRadius);
    QVector2D texelSize(0.0f, 0.0f);
    if (enableShadows && shadowMapSize > 0)
        texelSize = QVector2D(1.0f / float(shadowMapSize), 1.0f / float(shadowMapSize));
    triangleProgram.setUniformValue("u_shadowTexelSize", texelSize);
    triangleProgram.setUniformValue("u_shadowMap", 0);

    if (enableShadows) {
        functions->glActiveTexture(GL_TEXTURE0);
        functions->glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
    } else {
        functions->glBindTexture(GL_TEXTURE_2D, 0);
    }

    triangleProgram.enableAttributeArray(0);
    triangleProgram.setAttributeBuffer(0, GL_FLOAT, offsetof(TriangleVertex, position), 3, sizeof(TriangleVertex));
    triangleProgram.enableAttributeArray(1);
    triangleProgram.setAttributeBuffer(1, GL_FLOAT, offsetof(TriangleVertex, normal), 3, sizeof(TriangleVertex));
    triangleProgram.enableAttributeArray(2);
    triangleProgram.setAttributeBuffer(2, GL_FLOAT, offsetof(TriangleVertex, color), 4, sizeof(TriangleVertex));

    functions->glDisable(GL_BLEND);
    functions->glEnable(GL_DEPTH_TEST);
    functions->glLineWidth(1.0f);
    for (int i = 0; i < 4; ++i) {
        if (i < clipPlaneCount)
            functions->glEnable(GL_CLIP_DISTANCE0 + i);
        else
            functions->glDisable(GL_CLIP_DISTANCE0 + i);
    }
}

void Renderer::ensureShadowResources(int resolution)
{
    if (!functions)
        return;
    int size = std::max(resolution, 64);
    if (shadowFramebuffer != 0 && shadowDepthTexture != 0 && shadowMapSize == size)
        return;

    releaseShadowResources();
    shadowMapSize = size;

    functions->glGenFramebuffers(1, &shadowFramebuffer);
    functions->glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    functions->glGenTextures(1, &shadowDepthTexture);
    functions->glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
    functions->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    functions->glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    functions->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTexture, 0);
    functions->glDrawBuffers(0, nullptr);
    functions->glReadBuffer(GL_NONE);

    GLenum status = functions->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        releaseShadowResources();
        shadowMapSize = 0;
    }

    functions->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    functions->glBindTexture(GL_TEXTURE_2D, 0);
}

bool Renderer::renderShadowMap()
{
    if (!functions)
        return false;
    if (!lightingOptions.shadowsEnabled || !lightingOptions.sunValid)
        return false;
    if (triangleVertices.empty() || !boundsValid)
        return false;

    ensurePrograms();
    ensureShadowResources(lightingOptions.shadowMapResolution);
    if (shadowFramebuffer == 0 || shadowDepthTexture == 0 || shadowMapSize <= 0)
        return false;

    uploadTriangleBufferIfNeeded();

    QVector3D sunDir = lightingOptions.sunDirection;
    if (sunDir.isNull())
        sunDir = QVector3D(0.3f, 0.8f, 0.6f);
    sunDir.normalize();

    QVector3D center = (boundsMin + boundsMax) * 0.5f;
    QVector3D diag = boundsMax - boundsMin;
    float radius = diag.length() * 0.5f;
    if (radius < 1.0f)
        radius = 1.0f;

    QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
    if (std::fabs(QVector3D::dotProduct(up, sunDir)) > 0.9f)
        up = QVector3D(0.0f, 0.0f, 1.0f);
    QVector3D eye = center - sunDir * (radius * 2.5f + 5.0f);

    lightViewMatrix.setToIdentity();
    lightViewMatrix.lookAt(eye, center, up);

    QVector3D corners[8] = {
        { boundsMin.x(), boundsMin.y(), boundsMin.z() },
        { boundsMax.x(), boundsMin.y(), boundsMin.z() },
        { boundsMin.x(), boundsMax.y(), boundsMin.z() },
        { boundsMax.x(), boundsMax.y(), boundsMin.z() },
        { boundsMin.x(), boundsMin.y(), boundsMax.z() },
        { boundsMax.x(), boundsMin.y(), boundsMax.z() },
        { boundsMin.x(), boundsMax.y(), boundsMax.z() },
        { boundsMax.x(), boundsMax.y(), boundsMax.z() }
    };

    float minX = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();
    for (const auto& corner : corners) {
        QVector4D t = lightViewMatrix * QVector4D(corner, 1.0f);
        minX = std::min(minX, t.x());
        maxX = std::max(maxX, t.x());
        minY = std::min(minY, t.y());
        maxY = std::max(maxY, t.y());
        minZ = std::min(minZ, t.z());
        maxZ = std::max(maxZ, t.z());
    }

    float margin = std::max(radius * 0.1f, 1.0f);
    minX -= margin;
    maxX += margin;
    minY -= margin;
    maxY += margin;

    float nearPlane = std::max(0.1f, -maxZ - margin);
    float farPlane = std::max(nearPlane + 1.0f, -minZ + margin);

    lightProjectionMatrix.setToIdentity();
    lightProjectionMatrix.ortho(minX, maxX, minY, maxY, nearPlane, farPlane);
    lightViewProjection = lightProjectionMatrix * lightViewMatrix;

    GLint prevViewport[4];
    functions->glGetIntegerv(GL_VIEWPORT, prevViewport);
    GLint prevFbo = 0;
    functions->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFbo);

    functions->glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    functions->glViewport(0, 0, shadowMapSize, shadowMapSize);
    functions->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    functions->glDisable(GL_BLEND);
    functions->glEnable(GL_DEPTH_TEST);
    functions->glClear(GL_DEPTH_BUFFER_BIT);

    QOpenGLVertexArrayObject::Binder binder(&shadowVao);
    triangleBuffer.bind();

    shadowProgram.bind();
    shadowProgram.setUniformValue("u_lightMVP", lightViewProjection);
    shadowProgram.setUniformValue("u_clipPlaneCount", clipPlaneCount);
    if (clipPlaneCount > 0)
        shadowProgram.setUniformValueArray("u_clipPlanes", clipPlanes.data(), clipPlaneCount);
    shadowProgram.enableAttributeArray(0);
    shadowProgram.setAttributeBuffer(0, GL_FLOAT, offsetof(TriangleVertex, position), 3, sizeof(TriangleVertex));

    if (functions) {
        for (int i = 0; i < 4; ++i) {
            if (i < clipPlaneCount)
                functions->glEnable(GL_CLIP_DISTANCE0 + i);
            else
                functions->glDisable(GL_CLIP_DISTANCE0 + i);
        }
    }

    functions->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangleVertices.size()));

    shadowProgram.disableAttributeArray(0);

    functions->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    functions->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    functions->glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    for (int i = 0; i < 4; ++i)
        functions->glDisable(GL_CLIP_DISTANCE0 + i);

    return true;
}

void Renderer::releaseShadowResources()
{
    if (!functions)
        return;
    if (shadowDepthTexture) {
        functions->glDeleteTextures(1, &shadowDepthTexture);
        shadowDepthTexture = 0;
    }
    if (shadowFramebuffer) {
        functions->glDeleteFramebuffers(1, &shadowFramebuffer);
        shadowFramebuffer = 0;
    }
}

int Renderer::flush()
{
    ensurePrograms();
    int draws = 0;

    if (!triangleVertices.empty()) {
        shadowMapReady = renderShadowMap();
    } else {
        shadowMapReady = false;
    }

    bool colorMaskDisabled = false;
    if (currentStyle == RenderStyle::HiddenLine) {
        if (!triangleVertices.empty()) {
            ensureTriangleState();
            if (functions) {
                functions->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                colorMaskDisabled = true;
            }
            functions->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangleVertices.size()));
            ++draws;
        }
        if (colorMaskDisabled && functions)
            functions->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    } else if (currentStyle != RenderStyle::Wireframe && !triangleVertices.empty()) {
        ensureTriangleState();
        functions->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangleVertices.size()));
        ++draws;
    }

    for (const auto& batch : lineBatches) {
        if (batch.vertices.empty())
            continue;
        if (batch.config.category == LineCategory::Edge && currentStyle == RenderStyle::Shaded)
            continue;
        ensureLineState(batch);
        functions->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(batch.vertices.size()));
        ++draws;
    }

    functions->glDisable(GL_BLEND);
    functions->glEnable(GL_DEPTH_TEST);
    functions->glLineWidth(1.0f);
    functions->glBindTexture(GL_TEXTURE_2D, 0);

    for (int i = 0; i < 4; ++i)
        functions->glDisable(GL_CLIP_DISTANCE0 + i);

    return draws;
}
