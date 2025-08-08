#pragma once

#include <QMainWindow>

class GLViewport;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void selectTool();
    void lineTool();
    void arcTool();
    void rectTool();
    void moveTool();
    void rotateTool();
    void scaleTool();

private:
    void createMenus();
    void createToolbars();
    void createDockPanels();

    GLViewport *viewport;
};

