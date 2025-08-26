#include "MainWindow.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QAction>
#include <QFileDialog>
#include <QIcon>
#include <QLineEdit>
#include <QLabel>
#include "GLViewport.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("FreeCrafter");
    resize(1280, 820);

    viewport = new GLViewport(this);
    setCentralWidget(viewport);

    toolManager = std::make_unique<ToolManager>(viewport->getGeometry(), viewport->getCamera());
    viewport->setToolManager(toolManager.get());

    createMenus();
    createToolbars();
    createDockPanels();

    // status bar
    hintLabel = new QLabel("Ready");
    measurementBox = new QLineEdit();
    measurementBox->setPlaceholderText("Measurements");
    statusBar()->addWidget(hintLabel, 1);
    statusBar()->addPermanentWidget(measurementBox, 0);
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New", this, &MainWindow::newFile);
    fileMenu->addAction("Open...", this, &MainWindow::openFile);
    fileMenu->addAction("Save...", this, &MainWindow::saveFile);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close);

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Undo");
    editMenu->addAction("Redo");

    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Zoom Extents");

    menuBar()->addMenu("&Camera");
    menuBar()->addMenu("&Draw");
    menuBar()->addMenu("&Tools");
    menuBar()->addMenu("&Window");
    menuBar()->addMenu("&Help");
}

void MainWindow::createToolbars()
{
    QToolBar* left = addToolBar("Tools");
    left->setToolButtonStyle(Qt::ToolButtonIconOnly);
    left->setIconSize(QSize(24,24));

    selectAction = left->addAction(QIcon(":/icons/select.png"), "Select", this, &MainWindow::activateSelect);
    sketchAction = left->addAction(QIcon(":/icons/line.png"), "Sketch", this, &MainWindow::activateSketch);
    extrudeAction = left->addAction(QIcon(":/icons/pushpull.png"), "Extrude", this, &MainWindow::activateExtrude);

    addToolBar(Qt::LeftToolBarArea, left);
}

void MainWindow::createDockPanels()
{
    QDockWidget* tray = new QDockWidget("Default Tray", this);
    tray->setAllowedAreas(Qt::RightDockWidgetArea);
    tray->setWidget(new QWidget(tray));
    addDockWidget(Qt::RightDockWidgetArea, tray);
}

void MainWindow::newFile()
{
    viewport->getGeometry()->clear();
    viewport->update();
}

void MainWindow::openFile()
{
    QString fn = QFileDialog::getOpenFileName(this, "Open FreeCrafter Model", QString(), "FreeCrafter (*.fcm)");
    if (fn.isEmpty()) return;
    viewport->getGeometry()->loadFromFile(fn.toStdString());
    viewport->update();
}

void MainWindow::saveFile()
{
    QString fn = QFileDialog::getSaveFileName(this, "Save FreeCrafter Model", QString(), "FreeCrafter (*.fcm)");
    if (fn.isEmpty()) return;
    viewport->getGeometry()->saveToFile(fn.toStdString());
}

void MainWindow::activateSelect(){ toolManager->activateTool("SelectionTool"); hintLabel->setText("Select: Click to select. Delete to remove."); }
void MainWindow::activateSketch(){ toolManager->activateTool("SketchTool"); hintLabel->setText("Sketch: Click points on ground plane. Second click finishes."); }
void MainWindow::activateExtrude(){ toolManager->activateTool("ExtrudeTool"); hintLabel->setText("Extrude: Click to extrude last curve by 1.0."); }
