#pragma once

#include <QMainWindow>
#include <QPointer>
#include <memory>

class GLViewport;
class QAction;
class QLabel;
class QDockWidget;
class QToolBar;
class QToolButton;
class QTabBar;
class QActionGroup;
class MeasurementWidget;
class NavigationPreferences;
class PalettePreferences;
class EnvironmentPanel;
class InspectorPanel;
class QUndoStack;
class QSize;

#include "HotkeyManager.h"
#include "Renderer.h"
#include "Tools/ToolManager.h"
#include "CameraController.h"
#include "Navigation/ViewPresetManager.h"
#include "SunSettings.h"
#include "ui/LeftToolPalette.h"
#include "ui/RightTray.h"
#include "ui/ViewportOverlay.h"

#include <QHash>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
    ~MainWindow() override;

private slots:
    void newFile();
    void openFile();
    void importExternalModel();
    void saveFile();
    void saveFileAs();
    void exportFile();
    void onUndo();
    void onRedo();
    void showPreferences();
    void showCommandPalette();
    void toggleRightDock();
    void setDarkTheme(bool enabled);
    void runTask();
    void toggleTerminalDock();
    void showGitPopover();
    void showKeyboardShortcuts();
    void activateSelect();
    void activateLine();
    void activateRectangle();
    void activateCircle();
    void activateArc();
    void activateCenterArc();
    void activateTangentArc();
    void activatePolygon();
    void activateRotatedRectangle();
    void activateFreehand();
    void activateBezier();
    void activateMove();
    void activateRotate();
    void activateScale();
    void activateExtrude();
    void activateSection();
    void activatePan();
    void activateOrbit();
    void activateZoom();
    void activateMeasure();
    void toggleGrid(bool enabled);
    void updateCursor(double x, double y, double z);
    void updateFrameStats(double fps, double frameMs, int drawCalls);
    void handleMeasurementCommit(const QString& value, const QString& unitSystem);
    void showViewSettingsDialog();

private:
    void createMenus();
    void createToolbars();
    void createDockPanels();
    void createLeftDock();
    void createRightDock();
    void customizeViewport();
    void createStatusBarWidgets();
    void registerShortcuts();
    void applyThemeStylesheet();
    void restoreWindowState();
    void persistWindowState();
    void setActiveTool(QAction* action, const QString& toolId, const QString& hint);
    QString navigationHintForTool(const QString& toolName) const;
    void refreshNavigationActionHints();
    void updateSelectionStatus();
    void updateThemeActionIcon();
    void setRenderStyle(Renderer::RenderStyle style);
    void applyStandardView(ViewPresetManager::StandardView view);
    void setProjectionMode(CameraController::ProjectionMode mode, bool showStatus = true);
    void updateViewPresetButtonLabel();
    void updateUndoRedoActionText();
    void handleViewportResize(const QSize& size);
    void persistViewSettings() const;
    void syncViewSettingsUI();
    void handleSunSettingsChanged(const SunSettings& settings);
    void updateShadowStatus(const SunSettings& previous, const SunSettings& current);
    void closeEvent(QCloseEvent* event) override;

    GLViewport* viewport = nullptr;
    std::unique_ptr<ToolManager> toolManager;
    std::unique_ptr<NavigationPreferences> navigationPrefs;
    std::unique_ptr<PalettePreferences> palettePrefs;
    MeasurementWidget* measurementWidget = nullptr;
    InspectorPanel* inspectorPanel = nullptr;
    QLabel* hintLabel = nullptr;
    QLabel* coordLabel = nullptr;
    QLabel* selectionLabel = nullptr;
    QLabel* frameLabel = nullptr;
    QLabel* taskLabel = nullptr;

    QPointer<QToolBar> primaryToolbar;
    QPointer<QTabBar> documentTabs;
    QPointer<QToolButton> renderStyleButton;
    QPointer<QToolButton> viewPresetToolButton;
    QDockWidget* leftDock_ = nullptr;
    QDockWidget* rightDock_ = nullptr;
    QWidget* viewportWidget_ = nullptr;
    ViewportOverlay* overlay_ = nullptr;
    RightTray* rightTray_ = nullptr;

    QAction* actionNew = nullptr;
    QAction* actionOpen = nullptr;
    QAction* actionImport = nullptr;
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
    QAction* actionZoomSelection = nullptr;
    QAction* actionToggleRightDock = nullptr;
    QAction* actionToggleTheme = nullptr;
    QAction* actionViewIso = nullptr;
    QAction* actionViewTop = nullptr;
    QAction* actionViewBottom = nullptr;
    QAction* actionViewFront = nullptr;
    QAction* actionViewBack = nullptr;
    QAction* actionViewLeft = nullptr;
    QAction* actionViewRight = nullptr;
    QAction* actionToggleProjection = nullptr;
    QAction* actionViewSettings = nullptr;
    QAction* actionRun = nullptr;
    QAction* actionTerminal = nullptr;
    QAction* actionGit = nullptr;
    QAction* actionSettings = nullptr;
    QAction* actionShortcuts = nullptr;
    QUndoStack* undoStack = nullptr;

    QAction* selectAction = nullptr;
    QAction* lineAction = nullptr;
    QAction* rectangleAction = nullptr;
    QAction* circleAction = nullptr;
    QAction* arcAction = nullptr;
    QAction* centerArcAction = nullptr;
    QAction* tangentArcAction = nullptr;
    QAction* polygonAction = nullptr;
    QAction* rotatedRectangleAction = nullptr;
    QAction* freehandAction = nullptr;
    QAction* bezierAction = nullptr;
    QAction* moveAction = nullptr;
    QAction* rotateAction = nullptr;
    QAction* scaleAction = nullptr;
    QAction* extrudeAction = nullptr;
    QAction* sectionAction = nullptr;
    QAction* panAction = nullptr;
    QAction* orbitAction = nullptr;
    QAction* zoomAction = nullptr;
    QAction* measureAction = nullptr;
    QAction* gridAction = nullptr;
    QAction* actionShowFrameStatsHud = nullptr;

    QActionGroup* renderStyleGroup = nullptr;
    QActionGroup* paletteActionGroup = nullptr;
    QAction* renderWireframeAction = nullptr;
    QAction* renderShadedAction = nullptr;
    QAction* renderShadedEdgesAction = nullptr;
    QAction* renderHiddenLineAction = nullptr;
    QAction* renderMonochromeAction = nullptr;
    QAction* actionViewHiddenGeometry = nullptr;

    QActionGroup* toolActionGroup = nullptr;

    HotkeyManager hotkeys;
    QHash<QString, QAction*> paletteActions;
    bool darkTheme = true;
    Renderer::RenderStyle renderStyleChoice = Renderer::RenderStyle::ShadedWithEdges;
    bool showHiddenGeometry = false;
    bool showFrameStatsHud = false;
    QString currentViewPresetId = QStringLiteral("iso");
    ViewPresetManager viewPresetManager;
    SunSettings sunSettings;
    EnvironmentPanel* environmentPanel = nullptr;
};
