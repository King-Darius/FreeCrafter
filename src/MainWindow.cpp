#include "MainWindow.h"

#include <QAction>
#include <QDockWidget>
#include <QIcon>
#include <QMenuBar>
#include <QToolBar>

#include "GLViewport.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("FreeSketch");
    resize(1200, 800);

    viewport = new GLViewport(this);
    setCentralWidget(viewport);

    createMenus();
    createToolbars();
    createDockPanels();
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New", this, SLOT(newFile()));
    fileMenu->addAction("Open", this, SLOT(openFile()));
    fileMenu->addAction("Save", this, SLOT(saveFile()));
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, SLOT(close()));

    menuBar()->addMenu("&Edit");
    menuBar()->addMenu("&View");
    menuBar()->addMenu("&Camera");
    menuBar()->addMenu("&Draw");
    menuBar()->addMenu("&Tools");
    menuBar()->addMenu("&Extensions");
    menuBar()->addMenu("&Window");
    menuBar()->addMenu("&Help");
}

void MainWindow::createToolbars() {
    QToolBar *mainToolbar = addToolBar("Main Tools");
    auto addTool = [&](const QString &icon, const QString &tip, const QKeySequence &shortcut, const char *slot) {
        QAction *act = mainToolbar->addAction(QIcon(":/icons/" + icon), tip, this, slot);
        act->setShortcut(shortcut);
        act->setToolTip(tip + " (" + shortcut.toString() + ")");
    };

    addTool("select.png", "Select", QKeySequence(Qt::Key_Space), SLOT(selectTool()));
    addTool("line.png", "Line", QKeySequence(Qt::Key_L), SLOT(lineTool()));
    addTool("arc.png", "Arc", QKeySequence(Qt::Key_A), SLOT(arcTool()));
    addTool("rectangle.png", "Rectangle", QKeySequence(Qt::Key_R), SLOT(rectTool()));
    addTool("move.png", "Move", QKeySequence(Qt::Key_M), SLOT(moveTool()));
    addTool("rotate.png", "Rotate", QKeySequence(Qt::Key_Q), SLOT(rotateTool()));
    addTool("scale.png", "Scale", QKeySequence(Qt::Key_S), SLOT(scaleTool()));
}

void MainWindow::createDockPanels() {
    QDockWidget *entityInfo = new QDockWidget("Entity Info", this);
    addDockWidget(Qt::RightDockWidgetArea, entityInfo);
}

void MainWindow::newFile() {}
void MainWindow::openFile() {}
void MainWindow::saveFile() {}

void MainWindow::selectTool() {}
void MainWindow::lineTool() {}
void MainWindow::arcTool() {}
void MainWindow::rectTool() {}
void MainWindow::moveTool() {}
void MainWindow::rotateTool() {}
void MainWindow::scaleTool() {}

