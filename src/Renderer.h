#pragma once

#include <QMatrix3x3>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QVector4D>
#include <vector>

class QOpenGLFunctions;

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

    Renderer();

    void initialize(QOpenGLFunctions* functions);
    void beginFrame(const QMatrix4x4& projection, const QMatrix4x4& view, RenderStyle style);

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

    QOpenGLFunctions* functions = nullptr;
    QOpenGLShaderProgram lineProgram;
    QOpenGLShaderProgram triangleProgram;
    QOpenGLBuffer lineBuffer;
    QOpenGLBuffer triangleBuffer;
    QOpenGLVertexArrayObject lineVao;
    QOpenGLVertexArrayObject triangleVao;

    std::vector<LineBatch> lineBatches;
    std::vector<TriangleVertex> triangleVertices;

    QMatrix4x4 mvp;
    QMatrix3x3 normalMatrix;
    QVector3D lightDir;
    RenderStyle currentStyle = RenderStyle::ShadedWithEdges;
    bool programsReady = false;
};

