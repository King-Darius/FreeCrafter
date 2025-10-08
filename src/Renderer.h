#pragma once

#include <QMatrix3x3>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QVector2D>
#include <QVector4D>
#include <vector>
#include <array>

class QOpenGLExtraFunctions;

class Renderer
{
public:
    enum class RenderStyle {
        Wireframe,
        Shaded,
        ShadedWithEdges,
        HiddenLine,
        Monochrome
    };

    enum class LineCategory {
        Generic,
        Edge,
        HiddenEdge
    };

    struct LightingOptions {
        QVector3D sunDirection = QVector3D(0.3f, 0.8f, 0.6f);
        bool sunValid = false;
        bool shadowsEnabled = false;
        float shadowBias = 0.0035f;
        float shadowStrength = 0.65f;
        int shadowMapResolution = 1024;
        int shadowSampleRadius = 1;
    };

    Renderer();
    ~Renderer();

    void initialize(QOpenGLExtraFunctions* functions);
    void beginFrame(const QMatrix4x4& projection, const QMatrix4x4& view, RenderStyle style);
    void setLightingOptions(const LightingOptions& options);
    void setClipPlanes(const std::vector<QVector4D>& planes);
    void setShowGrid(bool visible);
    bool isGridVisible() const;

    void addLineSegments(const std::vector<QVector3D>& segments,
                         const QVector4D& color,
                         float width,
                         bool depthTest,
                         bool blend,
                         LineCategory category = LineCategory::Generic,
                         bool stippled = false,
                         float stippleScale = 8.0f);

    void addLineStrip(const std::vector<QVector3D>& points,
                      const QVector4D& color,
                      float width,
                      bool closed,
                      bool depthTest,
                      bool blend,
                      LineCategory category = LineCategory::Generic,
                      bool stippled = false,
                      float stippleScale = 8.0f);

    void addTriangle(const QVector3D& a,
                     const QVector3D& b,
                     const QVector3D& c,
                     const QVector3D& normal,
                     const QVector4D& color);

    int flush();

private:
    struct LineVertex {
        QVector3D position;
        QVector4D color;
    };

    struct TriangleVertex {
        QVector3D position;
        QVector3D normal;
        QVector4D color;
    };

    struct LineBatchConfig {
        float width = 1.0f;
        bool depthTest = true;
        bool blend = false;
        LineCategory category = LineCategory::Generic;
        bool stippled = false;
        float stippleScale = 8.0f;
    };

    struct LineBatch {
        LineBatchConfig config;
        std::vector<LineVertex> vertices;
    };

    LineBatch& fetchBatch(float width,
                          bool depthTest,
                          bool blend,
                          LineCategory category,
                          bool stippled,
                          float stippleScale);

    void ensurePrograms();
    void ensureLineState(const LineBatch& batch);
    void ensureTriangleState();
    void uploadTriangleBufferIfNeeded();
    void ensureShadowResources(int resolution);
    bool renderShadowMap();
    void releaseShadowResources();
    void expandBounds(const QVector3D& point);

    QOpenGLExtraFunctions* functions = nullptr;
    QOpenGLShaderProgram lineProgram;
    QOpenGLShaderProgram triangleProgram;
    QOpenGLShaderProgram shadowProgram;
    QOpenGLBuffer lineBuffer;
    QOpenGLBuffer triangleBuffer;
    QOpenGLVertexArrayObject lineVao;
    QOpenGLVertexArrayObject triangleVao;
    QOpenGLVertexArrayObject shadowVao;

    std::vector<LineBatch> lineBatches;
    std::vector<TriangleVertex> triangleVertices;

    LightingOptions lightingOptions;
    bool shadowMapReady = false;
    unsigned int shadowFramebuffer = 0;
    unsigned int shadowDepthTexture = 0;
    int shadowMapSize = 0;
    QMatrix4x4 lightViewMatrix;
    QMatrix4x4 lightProjectionMatrix;
    QMatrix4x4 lightViewProjection;
    QVector3D boundsMin;
    QVector3D boundsMax;
    bool boundsValid = false;
    bool triangleBufferDirty = true;

    std::array<QVector4D, 4> clipPlanes;
    int clipPlaneCount = 0;

    QMatrix4x4 mvp;
    QMatrix3x3 normalMatrix;
    QVector3D lightDir;
    RenderStyle currentStyle = RenderStyle::ShadedWithEdges;
    bool programsReady = false;
    bool gridVisible = true;
};


