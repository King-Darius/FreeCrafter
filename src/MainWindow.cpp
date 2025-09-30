#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QFileInfo>
#include <cmath>

#include "GLViewport.h"
#include "ui/MeasurementWidget.h"

namespace {
constexpr int kToolbarHeight = 48;
constexpr int kRibbonWidth = 56;
constexpr int kStatusHeight = 28;
const char* kSettingsGroup = "MainWindow";
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("FreeCrafter");
    resize(1440, 900);

    QWidget* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    documentTabs = new QTabBar(central);
    documentTabs->setDocumentMode(true);
    documentTabs->setExpanding(false);
    documentTabs->setDrawBase(false);
    documentTabs->setElideMode(Qt::ElideRight);
    documentTabs->setUsesScrollButtons(true);
    documentTabs->setFocusPolicy(Qt::NoFocus);
    documentTabs->setAccessibleName(tr("Document Tabs"));
    documentTabs->addTab(tr("Untitled •"));
    centralLayout->addWidget(documentTabs);

    viewport = new GLViewport(central);
    centralLayout->addWidget(viewport, 1);

    setCentralWidget(central);

    toolManager = std::make_unique<ToolManager>(viewport->getGeometry(), viewport->getCamera());
    viewport->setToolManager(toolManager.get());

    connect(viewport, &GLViewport::cursorPositionChanged, this, &MainWindow::updateCursor);
    connect(viewport, &GLViewport::frameStatsUpdated, this, &MainWindow::updateFrameStats);

    // Load persisted theme choice before applying styles
    {
        QSettings settings("FreeCrafter", "FreeCrafter");
        darkTheme = settings.value(QStringLiteral("%1/darkTheme").arg(kSettingsGroup), true).toBool();
    }

    createMenus();
    createToolbars();
    createDockPanels();
    createStatusBarWidgets();
    registerShortcuts();
    applyThemeStylesheet();
    restoreWindowState();
}

MainWindow::~MainWindow()
{
    persistWindowState();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    persistWindowState();
    QMainWindow::closeEvent(event);
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    actionNew = fileMenu->addAction(tr("New"), this, &MainWindow::newFile);
    actionNew->setIcon(QIcon(QStringLiteral(":/icons/newfile.svg")));
    actionNew->setStatusTip(tr("Create a blank FreeCrafter document"));

    actionOpen = fileMenu->addAction(tr("Open…"), this, &MainWindow::openFile);
    actionOpen->setIcon(QIcon(QStringLiteral(":/icons/open.svg")));
    actionOpen->setStatusTip(tr("Open an existing FreeCrafter document"));

    actionSave = fileMenu->addAction(tr("Save"), this, &MainWindow::saveFile);
    actionSave->setIcon(QIcon(QStringLiteral(":/icons/save.svg")));
    actionSave->setStatusTip(tr("Save the active document"));

    actionSaveAs = fileMenu->addAction(tr("Save As…"), this, &MainWindow::saveFileAs);
    actionSaveAs->setStatusTip(tr("Save the active document to a new file"));

    QMenu* recentMenu = fileMenu->addMenu(tr("Recent"));
    QAction* recentPlaceholder = recentMenu->addAction(tr("No recent files yet"));
    recentPlaceholder->setEnabled(false);

    actionExport = fileMenu->addAction(tr("Export…"), this, &MainWindow::exportFile);
    actionExport->setIcon(QIcon(QStringLiteral(":/icons/export.svg")));
    actionExport->setStatusTip(tr("Export geometry to an interchange format"));

    fileMenu->addSeparator();
    actionExit = fileMenu->addAction(tr("Exit"), this, &QWidget::close);
    actionExit->setStatusTip(tr("Close FreeCrafter"));

    QMenu* editMenu = menuBar()->addMenu(tr("Edit"));
    actionUndo = editMenu->addAction(tr("Undo"), this, &MainWindow::onUndo);
    actionUndo->setIcon(QIcon(QStringLiteral(":/icons/undo.svg")));
    actionRedo = editMenu->addAction(tr("Redo"), this, &MainWindow::onRedo);
    actionRedo->setIcon(QIcon(QStringLiteral(":/icons/redo.svg")));
    editMenu->addSeparator();
    actionPreferences = editMenu->addAction(tr("Preferences"), this, &MainWindow::showPreferences);

    QMenu* viewMenu = menuBar()->addMenu(tr("View"));
    actionZoomIn = viewMenu->addAction(tr("Zoom In"));
    actionZoomIn->setIcon(QIcon(QStringLiteral(":/icons/zoom_in.svg")));
    connect(actionZoomIn, &QAction::triggered, this, [this]() {
        statusBar()->showMessage(tr("Zoom In not implemented"), 1500);
    });
    actionZoomOut = viewMenu->addAction(tr("Zoom Out"));
    actionZoomOut->setIcon(QIcon(QStringLiteral(":/icons/zoom_out.svg")));
    connect(actionZoomOut, &QAction::triggered, this, [this]() {
        statusBar()->showMessage(tr("Zoom Out not implemented"), 1500);
    });
    actionZoomExtents = viewMenu->addAction(tr("Zoom Extents"));
    connect(actionZoomExtents, &QAction::triggered, this, [this]() {
        statusBar()->showMessage(tr("Zoom Extents not implemented"), 1500);
    });
    QAction* actionSplitView = viewMenu->addAction(tr("Split View"), this, [this]() {
        statusBar()->showMessage(tr("Split view is coming soon"), 2000);
    });
    actionSplitView->setEnabled(false);
    actionToggleRightDock = viewMenu->addAction(tr("Toggle Right Panel"), this, &MainWindow::toggleRightDock);
    actionToggleTheme = viewMenu->addAction(tr("Toggle Theme"), this, &MainWindow::toggleTheme);

    QMenu* insertMenu = menuBar()->addMenu(tr("Insert"));
    insertMenu->addAction(tr("Shapes"), this, [this]() { statusBar()->showMessage(tr("Insert Shapes not implemented"), 2000); });
    insertMenu->addAction(tr("Guides"), this, [this]() { statusBar()->showMessage(tr("Insert Guides not implemented"), 2000); });
    insertMenu->addAction(tr("Images"), this, [this]() { statusBar()->showMessage(tr("Insert Images not implemented"), 2000); });
    insertMenu->addAction(tr("External Reference"), this, [this]() { statusBar()->showMessage(tr("External references not implemented"), 2000); });

    QMenu* toolsMenu = menuBar()->addMenu(tr("Tools"));
    toolsMenu->addAction(tr("Plugins…"), this, [this]() { statusBar()->showMessage(tr("Plugin manager not implemented"), 2000); });
    actionPalette = toolsMenu->addAction(tr("Command Palette…"), this, &MainWindow::showCommandPalette);
    actionPalette->setIcon(QIcon(QStringLiteral(":/icons/search.svg")));

    QMenu* windowMenu = menuBar()->addMenu(tr("Window"));
    windowMenu->addAction(tr("New Window"), this, [this]() { statusBar()->showMessage(tr("New window not implemented"), 2000); });

    QMenu* helpMenu = menuBar()->addMenu(tr("Help"));
    actionShortcuts = helpMenu->addAction(tr("Keyboard Shortcuts"), this, &MainWindow::showKeyboardShortcuts);
    actionShortcuts->setIcon(QIcon(QStringLiteral(":/icons/help.svg")));
    helpMenu->addAction(tr("About"), this, [this]() {
        QMessageBox::about(this, tr("About FreeCrafter"), tr("FreeCrafter — MIT Licensed CAD shell prototype."));
    });
}

void MainWindow::createToolbars()
{
    primaryToolbar = addToolBar(tr("Primary"));
    primaryToolbar->setMovable(false);
    primaryToolbar->setFixedHeight(kToolbarHeight);
    primaryToolbar->setIconSize(QSize(20, 20));
    primaryToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    primaryToolbar->addAction(actionNew);
    primaryToolbar->addAction(actionOpen);
    primaryToolbar->addAction(actionSave);
    primaryToolbar->addSeparator();
    primaryToolbar->addAction(actionUndo);
    primaryToolbar->addAction(actionRedo);
    primaryToolbar->addSeparator();
    primaryToolbar->addAction(actionPalette);
    actionRun = primaryToolbar->addAction(QIcon(QStringLiteral(":/icons/run.svg")), tr("Run"), this, &MainWindow::runTask);
    actionRun->setStatusTip(tr("Execute the current task"));
    actionTerminal = primaryToolbar->addAction(QIcon(QStringLiteral(":/icons/terminal.svg")), tr("Terminal"), this, &MainWindow::toggleTerminalDock);
    actionTerminal->setStatusTip(tr("Toggle terminal dock"));
    actionGit = primaryToolbar->addAction(QIcon(QStringLiteral(":/icons/git.svg")), tr("Git"), this, &MainWindow::showGitPopover);
    actionGit->setStatusTip(tr("Open Git controls"));
    primaryToolbar->addSeparator();
    primaryToolbar->addAction(actionToggleTheme);
    actionSettings = primaryToolbar->addAction(QIcon(QStringLiteral(":/icons/settings.svg")), tr("Settings"), this, &MainWindow::showPreferences);

    toolRibbon = new QToolBar(tr("Tools"), this);
    toolRibbon->setOrientation(Qt::Vertical);
    toolRibbon->setMovable(false);
    toolRibbon->setIconSize(QSize(20, 20));
    toolRibbon->setFixedWidth(kRibbonWidth);
    toolRibbon->setToolButtonStyle(Qt::ToolButtonIconOnly);

    toolActionGroup = new QActionGroup(this);
    toolActionGroup->setExclusive(true);

    selectAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/select.png")), tr("Select"), this, &MainWindow::activateSelect);
    selectAction->setCheckable(true);
    toolActionGroup->addAction(selectAction);

    lineAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/line.png")), tr("Line"), this, &MainWindow::activateLine);
    lineAction->setCheckable(true);
    toolActionGroup->addAction(lineAction);

    moveAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/move.png")), tr("Move"), this, &MainWindow::activateMove);
    moveAction->setCheckable(true);
    toolActionGroup->addAction(moveAction);

    rotateAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/rotate.png")), tr("Rotate"), this, &MainWindow::activateRotate);
    rotateAction->setCheckable(true);
    toolActionGroup->addAction(rotateAction);

    scaleAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/scale.png")), tr("Scale"), this, &MainWindow::activateScale);
    scaleAction->setCheckable(true);
    toolActionGroup->addAction(scaleAction);

    extrudeAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/pushpull.png")), tr("Extrude"), this, &MainWindow::activateExtrude);
    extrudeAction->setCheckable(true);
    toolActionGroup->addAction(extrudeAction);

    toolRibbon->addSeparator();

    panAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/pan.svg")), tr("Pan"), this, &MainWindow::activatePan);
    panAction->setCheckable(true);
    toolActionGroup->addAction(panAction);

    orbitAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/orbit.svg")), tr("Orbit"), this, &MainWindow::activateOrbit);
    orbitAction->setCheckable(true);
    toolActionGroup->addAction(orbitAction);

    measureAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/measure.svg")), tr("Measure"), this, &MainWindow::activateMeasure);
    measureAction->setCheckable(true);
    toolActionGroup->addAction(measureAction);

    gridAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/grid.svg")), tr("Toggle Grid"), this, &MainWindow::toggleGrid);
    gridAction->setCheckable(true);
    gridAction->setChecked(true);

    addToolBar(Qt::LeftToolBarArea, toolRibbon);

    selectAction->setChecked(true);
    activateSelect();
}

void MainWindow::createDockPanels()
{
    rightDock = new QDockWidget(tr("Panels"), this);
    rightDock->setAllowedAreas(Qt::RightDockWidgetArea);
    rightDock->setMinimumWidth(280);
    rightDock->setMaximumWidth(420);
    rightDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    rightTabs = new QTabWidget(rightDock);
    rightTabs->setDocumentMode(true);

    auto makePlaceholder = [](const QString& title) {
        QWidget* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->addStretch();
        auto* label = new QLabel(QObject::tr("%1 panel placeholder").arg(title));
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
        layout->addStretch();
        return page;
    };

    rightTabs->addTab(makePlaceholder(tr("Inspector")), tr("Inspector"));
    rightTabs->addTab(makePlaceholder(tr("Explorer")), tr("Explorer"));
    rightTabs->addTab(makePlaceholder(tr("History")), tr("History"));

    rightDock->setWidget(rightTabs);
    addDockWidget(Qt::RightDockWidgetArea, rightDock);
}

void MainWindow::createStatusBarWidgets()
{
    statusBar()->setSizeGripEnabled(true);
    statusBar()->setMinimumHeight(kStatusHeight);

    coordLabel = new QLabel(tr("X: --  Y: --  Z: --"), this);
    selectionLabel = new QLabel(tr("Selection: 0 items"), this);
    hintLabel = new QLabel(tr("Ready"), this);
    taskLabel = new QLabel(tr("No background tasks"), this);

    measurementWidget = new MeasurementWidget(this);
    connect(measurementWidget, &MeasurementWidget::measurementCommitted, this, &MainWindow::handleMeasurementCommit);

    statusBar()->addWidget(coordLabel);
    statusBar()->addWidget(selectionLabel);
    statusBar()->addPermanentWidget(hintLabel, 1);
    statusBar()->addPermanentWidget(measurementWidget, 0);
    statusBar()->addPermanentWidget(taskLabel, 0);
}

void MainWindow::registerShortcuts()
{
    hotkeys.registerAction(QStringLiteral("file.new"), actionNew);
    hotkeys.registerAction(QStringLiteral("file.open"), actionOpen);
    hotkeys.registerAction(QStringLiteral("file.save"), actionSave);
    hotkeys.registerAction(QStringLiteral("file.saveAs"), actionSaveAs);
    hotkeys.registerAction(QStringLiteral("file.export"), actionExport);
    hotkeys.registerAction(QStringLiteral("file.exit"), actionExit);
    hotkeys.registerAction(QStringLiteral("edit.undo"), actionUndo);
    hotkeys.registerAction(QStringLiteral("edit.redo"), actionRedo);
    hotkeys.registerAction(QStringLiteral("edit.preferences"), actionPreferences);
    hotkeys.registerAction(QStringLiteral("palette.show"), actionPalette);
    hotkeys.registerAction(QStringLiteral("view.zoomIn"), actionZoomIn);
    hotkeys.registerAction(QStringLiteral("view.zoomOut"), actionZoomOut);
    hotkeys.registerAction(QStringLiteral("view.zoomExtents"), actionZoomExtents);
    hotkeys.registerAction(QStringLiteral("view.toggleRightDock"), actionToggleRightDock);
    hotkeys.registerAction(QStringLiteral("theme.toggle"), actionToggleTheme);
    hotkeys.registerAction(QStringLiteral("tools.select"), selectAction);
    hotkeys.registerAction(QStringLiteral("tools.line"), lineAction);
    hotkeys.registerAction(QStringLiteral("tools.move"), moveAction);
    hotkeys.registerAction(QStringLiteral("tools.rotate"), rotateAction);
    hotkeys.registerAction(QStringLiteral("tools.scale"), scaleAction);
    hotkeys.registerAction(QStringLiteral("tools.extrude"), extrudeAction);
    hotkeys.registerAction(QStringLiteral("tools.pan"), panAction);
    hotkeys.registerAction(QStringLiteral("tools.orbit"), orbitAction);
    hotkeys.registerAction(QStringLiteral("tools.measure"), measureAction);
    hotkeys.registerAction(QStringLiteral("tools.grid"), gridAction);
    hotkeys.registerAction(QStringLiteral("help.shortcuts"), actionShortcuts);

    auto updateTooltips = [this]() {
        auto format = [](QAction* action, const QString& label) {
            if (!action)
                return label;
            const QString shortcut = action->shortcut().toString(QKeySequence::NativeText);
            return shortcut.isEmpty() ? label : QStringLiteral("%1 (%2)").arg(label, shortcut);
        };

        if (actionNew) actionNew->setToolTip(format(actionNew, tr("New")));
        if (actionOpen) actionOpen->setToolTip(format(actionOpen, tr("Open")));
        if (actionSave) actionSave->setToolTip(format(actionSave, tr("Save")));
        if (actionUndo) actionUndo->setToolTip(format(actionUndo, tr("Undo")));
        if (actionRedo) actionRedo->setToolTip(format(actionRedo, tr("Redo")));
        if (actionPalette) actionPalette->setToolTip(format(actionPalette, tr("Command Palette")));
        if (actionToggleTheme) actionToggleTheme->setToolTip(format(actionToggleTheme, tr("Toggle Theme")));
        if (selectAction) selectAction->setToolTip(format(selectAction, tr("Select")));
        if (lineAction) lineAction->setToolTip(format(lineAction, tr("Line")));
        if (moveAction) moveAction->setToolTip(format(moveAction, tr("Move")));
        if (rotateAction) rotateAction->setToolTip(format(rotateAction, tr("Rotate")));
        if (scaleAction) scaleAction->setToolTip(format(scaleAction, tr("Scale")));
        if (panAction) panAction->setToolTip(format(panAction, tr("Pan")));
        if (orbitAction) orbitAction->setToolTip(format(orbitAction, tr("Orbit")));
        if (extrudeAction) extrudeAction->setToolTip(format(extrudeAction, tr("Extrude")));
        if (measureAction) measureAction->setToolTip(format(measureAction, tr("Measure")));
        if (gridAction) gridAction->setToolTip(format(gridAction, tr("Toggle Grid")));
    };

    updateTooltips();
    connect(&hotkeys, &HotkeyManager::shortcutsChanged, this, updateTooltips);
}

void MainWindow::applyThemeStylesheet()
{
    const QString path = darkTheme ? QStringLiteral(":/styles/app.qss") : QStringLiteral(":/styles/app_light.qss");
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
    }
    updateThemeActionIcon();
}

void MainWindow::restoreWindowState()
{
    QSettings settings("FreeCrafter", "FreeCrafter");
    settings.beginGroup(kSettingsGroup);
    const QByteArray geometry = settings.value("geometry").toByteArray();
    const QByteArray state = settings.value("state").toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
    if (!state.isEmpty())
        restoreState(state);
    settings.endGroup();
}

void MainWindow::persistWindowState()
{
    QSettings settings("FreeCrafter", "FreeCrafter");
    settings.beginGroup(kSettingsGroup);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.setValue("darkTheme", darkTheme);
    settings.endGroup();
}

void MainWindow::setActiveTool(QAction* action, const QString& toolId, const QString& hint)
{
    if (!action)
        return;
    if (!toolManager)
        return;
    if (!toolId.isEmpty())
        toolManager->activateTool(toolId.toUtf8().constData());
    action->setChecked(true);
    if (hintLabel)
        hintLabel->setText(hint);
}

void MainWindow::updateThemeActionIcon()
{
    if (!actionToggleTheme)
        return;
    const QString iconPath = darkTheme ? QStringLiteral(":/icons/theme_light.svg") : QStringLiteral(":/icons/theme_dark.svg");
    actionToggleTheme->setIcon(QIcon(iconPath));
}

void MainWindow::newFile()
{
    viewport->getGeometry()->clear();
    viewport->update();
    statusBar()->showMessage(tr("New document created"), 1500);
}

void MainWindow::openFile()
{
    const QString fn = QFileDialog::getOpenFileName(this, tr("Open FreeCrafter Model"), QString(), tr("FreeCrafter (*.fcm)"));
    if (fn.isEmpty()) return;
    viewport->getGeometry()->loadFromFile(fn.toStdString());
    viewport->update();
    statusBar()->showMessage(tr("Opened %1").arg(QFileInfo(fn).fileName()), 1500);
}

void MainWindow::saveFile()
{
    const QString fn = QFileDialog::getSaveFileName(this, tr("Save FreeCrafter Model"), QString(), tr("FreeCrafter (*.fcm)"));
    if (fn.isEmpty()) return;
    viewport->getGeometry()->saveToFile(fn.toStdString());
    statusBar()->showMessage(tr("Saved %1").arg(QFileInfo(fn).fileName()), 1500);
}

void MainWindow::saveFileAs()
{
    saveFile();
}

void MainWindow::exportFile()
{
    statusBar()->showMessage(tr("Export workflow not implemented"), 2000);
}

void MainWindow::onUndo()
{
    statusBar()->showMessage(tr("Undo not wired yet"), 1500);
}

void MainWindow::onRedo()
{
    statusBar()->showMessage(tr("Redo not wired yet"), 1500);
}

void MainWindow::showPreferences()
{
    showKeyboardShortcuts();
}

void MainWindow::showCommandPalette()
{
    statusBar()->showMessage(tr("Command palette coming soon"), 2000);
}

void MainWindow::toggleRightDock()
{
    if (rightDock)
        rightDock->setVisible(!rightDock->isVisible());
}

void MainWindow::toggleTheme()
{
    darkTheme = !darkTheme;
    applyThemeStylesheet();
    persistWindowState();
}

void MainWindow::runTask()
{
    taskLabel->setText(tr("Running task…"));
    QTimer::singleShot(1200, this, [this]() {
        taskLabel->setText(tr("No background tasks"));
    });
}

void MainWindow::toggleTerminalDock()
{
    statusBar()->showMessage(tr("Terminal dock not implemented"), 2000);
}

void MainWindow::showGitPopover()
{
    statusBar()->showMessage(tr("Git popover placeholder"), 2000);
}

void MainWindow::showKeyboardShortcuts()
{
    hotkeys.showEditor(this);
}

void MainWindow::activateSelect()
{
    setActiveTool(selectAction, QStringLiteral("SmartSelectTool"), tr("Select: Click to pick, drag left-to-right for window, right-to-left for crossing."));
}

void MainWindow::activateLine()
{
    setActiveTool(lineAction, QStringLiteral("LineTool"), tr("Line: Click to place vertices. Press Enter to finish, Esc to cancel."));
}

void MainWindow::activateMove()
{
    setActiveTool(moveAction, QStringLiteral("MoveTool"), tr("Move: Drag to translate selection. Use arrow keys for axis locks."));
}

void MainWindow::activateRotate()
{
    setActiveTool(rotateAction, QStringLiteral("RotateTool"), tr("Rotate: Drag to rotate around pivot. Use snaps or axis locks."));
}

void MainWindow::activateScale()
{
    setActiveTool(scaleAction, QStringLiteral("ScaleTool"), tr("Scale: Drag to scale selection. Axis locks limit scaling direction."));
}

void MainWindow::activateExtrude()
{
    setActiveTool(extrudeAction, QStringLiteral("ExtrudeTool"), tr("Extrude: Click to extrude last curve by 1.0."));
}

void MainWindow::activatePan()
{
    setActiveTool(panAction, QString(), tr("Pan: Hold middle mouse button or Space+drag"));
}

void MainWindow::activateOrbit()
{
    setActiveTool(orbitAction, QString(), tr("Orbit: Hold right mouse or O+drag"));
}

void MainWindow::activateMeasure()
{
    setActiveTool(measureAction, QString(), tr("Measure: Click two points to sample distance"));
}

void MainWindow::toggleGrid()
{
    if (hintLabel)
        hintLabel->setText(gridAction->isChecked() ? tr("Grid enabled") : tr("Grid hidden"));
    viewport->update();
}

void MainWindow::updateCursor(double x, double y, double z)
{
    if (!coordLabel)
        return;
    if (std::isnan(x) || std::isnan(y) || std::isnan(z)) {
        coordLabel->setText(tr("X: --  Y: --  Z: --"));
        return;
    }
    coordLabel->setText(tr("X: %1  Y: %2  Z: %3").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(z, 0, 'f', 2));
}

void MainWindow::updateFrameStats(double fps, double frameMs, int drawCalls)
{
    if (selectionLabel)
        selectionLabel->setText(tr("Frame: %1 ms • %2 draws • %3 fps").arg(frameMs, 0, 'f', 2).arg(drawCalls).arg(fps, 0, 'f', 1));
}

void MainWindow::handleMeasurementCommit(const QString& value, const QString& unitSystem)
{
    statusBar()->showMessage(tr("Measurement override %1 (%2) not wired yet").arg(value, unitSystem), 2000);
}
