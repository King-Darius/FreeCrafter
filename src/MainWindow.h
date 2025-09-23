#pragma once

#include <QMainWindow>
#include <QPointer>
#include <memory>

class GLViewport;
class QAction;
class QLabel;
class QDockWidget;
class QToolBar;
class QTabWidget;
class QTabBar;
class QActionGroup;
class MeasurementWidget;

#include "HotkeyManager.h"
#include "Tools/ToolManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
    ~MainWindow() override;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void exportFile();
    void onUndo();
    void onRedo();
    void showPreferences();
    void showCommandPalette();
    void toggleRightDock();
    void toggleTheme();
    void runTask();
    void toggleTerminalDock();
    void showGitPopover();
    void showKeyboardShortcuts();
    void activateSelect();
    void activateSketch();
    void activateExtrude();
    void activatePan();
    void activateOrbit();
    void activateMeasure();
    void toggleGrid();
    void updateCursor(double x, double y, double z);
    void updateFrameStats(double fps, double frameMs, int drawCalls);
    void handleMeasurementCommit(const QString& value, const QString& unitSystem);

private:
    void createMenus();
    void createToolbars();
    void createDockPanels();
    void createStatusBarWidgets();
    void registerShortcuts();
    void applyThemeStylesheet();
    void restoreWindowState();
    void persistWindowState();
    void setActiveTool(QAction* action, const QString& toolId, const QString& hint);
    void updateThemeActionIcon();

    void closeEvent(QCloseEvent* event) override;

    GLViewport* viewport = nullptr;
    std::unique_ptr<ToolManager> toolManager;
    MeasurementWidget* measurementWidget = nullptr;
    QLabel* hintLabel = nullptr;
    QLabel* coordLabel = nullptr;
    QLabel* selectionLabel = nullptr;
    QLabel* taskLabel = nullptr;

    QPointer<QToolBar> primaryToolbar;
    QPointer<QToolBar> toolRibbon;
    QPointer<QDockWidget> rightDock;
    QPointer<QTabWidget> rightTabs;
    QPointer<QTabBar> documentTabs;

    QAction* actionNew = nullptr;
    QAction* actionOpen = nullptr;
    QAction* actionSave = nullptr;
    QAction* actionSaveAs = nullptr;
    QAction* actionExport = nullptr;
    QAction* actionExit = nullptr;
    QAction* actionUndo = nullptr;
    QAction* actionRedo = nullptr;
    QAction* actionPreferences = nullptr;
    QAction* actionPalette = nullptr;
    QAction* actionZoomIn = nullptr;
    QAction* actionZoomOut = nullptr;
    QAction* actionZoomExtents = nullptr;
    QAction* actionToggleRightDock = nullptr;
    QAction* actionToggleTheme = nullptr;
    QAction* actionRun = nullptr;
    QAction* actionTerminal = nullptr;
    QAction* actionGit = nullptr;
    QAction* actionSettings = nullptr;
    QAction* actionShortcuts = nullptr;

    QAction* selectAction = nullptr;
    QAction* sketchAction = nullptr;
    QAction* extrudeAction = nullptr;
    QAction* panAction = nullptr;
    QAction* orbitAction = nullptr;
    QAction* measureAction = nullptr;
    QAction* gridAction = nullptr;

    QActionGroup* toolActionGroup = nullptr;

    HotkeyManager hotkeys;
    bool darkTheme = true;
};
