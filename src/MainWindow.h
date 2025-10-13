#pragma once

#include <QMainWindow>
#include <QPointer>
#include <QString>
#include <memory>
#include <vector>

class GLViewport;
class QAction;
class QLabel;
class QDockWidget;
class QToolBar;
class QToolButton;
class QTabBar;
class QActionGroup;
class QMenu;
class QStackedWidget;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QCheckBox;
class MeasurementWidget;
class NavigationPreferences;
class PalettePreferences;
class EnvironmentPanel;
class InspectorPanel;
class TerminalDock;
class QUndoStack;
class QSize;
class AutosaveManager;

namespace Phase6 {
struct RoundCornerOptions;
struct LoftOptions;
}

#include "HotkeyManager.h"
#include "Renderer.h"
#include "Tools/ToolManager.h"
#include "Tools/ToolRegistry.h"
#include "CameraController.h"
#include "Core/CommandStack.h"
#include "Navigation/ViewPresetManager.h"
#include "SunSettings.h"
#include "ui/LeftToolPalette.h"
#include "ui/RightTray.h"
#include "ui/ViewportOverlay.h"

#include <QHash>
#include <QVector>

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
    void showInsertShapesDialog();
    void showGuideManager();
    void showImageImportDialog();
    void showExternalReferenceDialog();
    void showPluginManagerDialog();
    void spawnNewWindow();
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
    void activateChamfer();
    void activateLoft();
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
    void applyGridSettings(double majorSpacing, int minorDivisions, int majorExtent);
    void applyShadowParameters(bool enabled, int qualityIndex, double strengthPercent, double biasValue);

private:
    void createMenus();
    void createToolbars();
    void createDockPanels();
    void createLeftDock();
    void createRightDock();
    void createTerminalDock();
    void customizeViewport();
    void createStatusBarWidgets();
    void registerShortcuts();
    void applyThemeStylesheet();
    void restoreWindowState();
    void persistWindowState();
    void persistAutosaveSettings() const;
    void setActiveTool(QAction* action, const QString& toolId, const QString& hint);
    void updateToolOptionsPanel(const QString& toolId);
    void syncActiveToolOptions();
    void updateChamferControls(const Phase6::RoundCornerOptions& options);
    void updateLoftControls(const Phase6::LoftOptions& options);
    void applyChamferDefaults();
    void applyLoftDefaults();
    void populateAdvancedToolsMenu();
    QString navigationHintForTool(const QString& toolName) const;
    void refreshNavigationActionHints();
    void activateToolByKey(ToolRegistry::ToolId key, QAction* action);
    QString toolHintForDescriptor(const ToolRegistry::ToolDescriptor& descriptor) const;
    void validateToolActionBindings() const;
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
    void initializeAutosave();
    void maybeRestoreAutosave();
    void updateAutosaveSource(const QString& path, bool purgePreviousPrefix);

    GLViewport* viewport = nullptr;
    std::unique_ptr<ToolManager> toolManager;
    std::unique_ptr<Core::CommandStack> commandStack;
    std::unique_ptr<NavigationPreferences> navigationPrefs;
    std::unique_ptr<PalettePreferences> palettePrefs;
    std::unique_ptr<AutosaveManager> autosaveManager;
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
    QDockWidget* terminalDock_ = nullptr;
    QWidget* viewportWidget_ = nullptr;
    ViewportOverlay* overlay_ = nullptr;
    RightTray* rightTray_ = nullptr;
    TerminalDock* terminalWidget_ = nullptr;

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
    QAction* actionPerspectiveProjection = nullptr;
    QAction* actionParallelProjection = nullptr;
    QAction* actionToggleProjection = nullptr;
    QAction* actionViewSettings = nullptr;
    QAction* actionShowSectionPlanes = nullptr;
    QAction* actionShowSectionFills = nullptr;
    QAction* actionShowGuides = nullptr;
    QAction* actionEnableShadows = nullptr;
    QAction* actionGridSettings = nullptr;
    QAction* actionRun = nullptr;
    QAction* actionTerminal = nullptr;
    QAction* actionGit = nullptr;
    QAction* actionSettings = nullptr;
    QAction* actionShortcuts = nullptr;
    QUndoStack* undoStack = nullptr;

    QString currentDocumentPath;

    QActionGroup* projectionModeGroup = nullptr;
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
    QAction* chamferAction = nullptr;
    QAction* loftAction = nullptr;
    QAction* sectionAction = nullptr;
    QAction* panAction = nullptr;
    QAction* orbitAction = nullptr;
    QAction* zoomAction = nullptr;
    QAction* measureAction = nullptr;
    QAction* chamferOptionsAction = nullptr;
    QAction* loftOptionsAction = nullptr;
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
    QVector<QPointer<MainWindow>> secondaryWindows;
    struct ToolActionBinding {
        const ToolRegistry::ToolDescriptor* descriptor = nullptr;
        QAction* action = nullptr;
    };
    std::vector<ToolActionBinding> toolActionBindings_;

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
    bool isRestoringWindowState_ = false;
    bool initializingTerminalDock_ = false;

    QToolBar* toolOptionsToolbar = nullptr;
    QStackedWidget* toolOptionsStack = nullptr;
    QWidget* toolOptionsPlaceholder = nullptr;
    QWidget* chamferOptionsWidget = nullptr;
    QWidget* loftOptionsWidget = nullptr;
    QDoubleSpinBox* chamferRadiusSpin = nullptr;
    QSpinBox* chamferSegmentsSpin = nullptr;
    QComboBox* chamferStyleCombo = nullptr;
    QCheckBox* chamferHardEdgeCheck = nullptr;
    QToolButton* chamferApplyButton = nullptr;
    QToolButton* chamferDialogButton = nullptr;
    QSpinBox* loftSectionsSpin = nullptr;
    QDoubleSpinBox* loftTwistSpin = nullptr;
    QSpinBox* loftSmoothingSpin = nullptr;
    QCheckBox* loftCloseRailsCheck = nullptr;
    QCheckBox* loftSmoothNormalsCheck = nullptr;
    QCheckBox* loftSymmetryCheck = nullptr;
    QToolButton* loftApplyButton = nullptr;
    QToolButton* loftDialogButton = nullptr;
    QMenu* advancedToolsMenu = nullptr;
    Phase6::RoundCornerOptions chamferDefaults_;
    Phase6::LoftOptions loftDefaults_;
};
