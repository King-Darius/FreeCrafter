#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class DebugSimpleViewport : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit DebugSimpleViewport(QWidget* parent = nullptr);
    ~DebugSimpleViewport() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initializeGeometry();
    void logContextInformation() const;

    QOpenGLShaderProgram program;
    QOpenGLBuffer vertexBuffer { QOpenGLBuffer::VertexBuffer };
    QOpenGLVertexArrayObject vao;
    bool geometryReady = false;
    QMatrix4x4 projection;
};

