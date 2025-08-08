#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class GLViewport : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GLViewport(QWidget *parent = nullptr);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
};

