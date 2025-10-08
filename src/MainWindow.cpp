#include "MainWindow.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "FileIO/Exporters/SceneExporter.h"
#include "GLViewport.h"
#include "NavigationConfig.h"
#include "NavigationPreferences.h"
#include "PalettePreferences.h"
#include "Scene/Document.h"
#include "SunModel.h"
#include "Tools/ToolManager.h"
#include "ui/EnvironmentPanel.h"
#include "ui/MeasurementWidget.h"
#include "ui/ViewSettingsDialog.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QStringList>

#include <cmath>

constexpr double kPi = 3.14159265358979323846;

namespace {

constexpr int kToolbarHeight = 48;

constexpr int kRibbonWidth = 56;

constexpr int kStatusHeight = 28;

const char* kSettingsGroup = "MainWindow";

QString renderStyleToSetting(Renderer::RenderStyle style)

{

    switch (style) {

    case Renderer::RenderStyle::Wireframe:

        return QStringLiteral("wireframe");

    case Renderer::RenderStyle::Shaded:

        return QStringLiteral("shaded");

    case Renderer::RenderStyle::ShadedWithEdges:

        return QStringLiteral("shadedEdges");

    case Renderer::RenderStyle::HiddenLine:

        return QStringLiteral("hiddenLine");

    case Renderer::RenderStyle::Monochrome:

        return QStringLiteral("monochrome");

    default:

        return QStringLiteral("shadedEdges");

    }

}

Renderer::RenderStyle renderStyleFromSetting(const QString& value)

{

    if (value == QLatin1String("wireframe")) {

        return Renderer::RenderStyle::Wireframe;

    }

    if (value == QLatin1String("shaded")) {

        return Renderer::RenderStyle::Shaded;

    }

    if (value == QLatin1String("hiddenLine")) {

        return Renderer::RenderStyle::HiddenLine;

    }

    if (value == QLatin1String("monochrome")) {

        return Renderer::RenderStyle::Monochrome;

    }

    return Renderer::RenderStyle::ShadedWithEdges;

}

struct MeasurementParseResult {

    bool ok = false;

    double value = 0.0;

    QString display;

    QString error;

};

QString removeWhitespace(QString text)

{

    text.remove(QChar(' '));

    text.remove(QChar('\t'));

    text.remove(QChar('\n'));

    text.remove(QChar('\r'));

    return text;

}

MeasurementParseResult parseImperialDistance(const QString& raw)

{

    MeasurementParseResult result;

    QString sanitized = removeWhitespace(raw.trimmed());

    if (sanitized.isEmpty()) {

        result.error = QObject::tr("empty measurement");

        return result;

    }

    bool negative = false;

    if (sanitized.startsWith(QLatin1Char('+')) || sanitized.startsWith(QLatin1Char('-'))) {

        negative = sanitized.startsWith(QLatin1Char('-'));

        sanitized = sanitized.mid(1);

    }

    sanitized.replace(QStringLiteral("ft"), QStringLiteral("'"), Qt::CaseInsensitive);

    sanitized.replace(QStringLiteral("in"), QStringLiteral("\""), Qt::CaseInsensitive);

    double feet = 0.0;

    double inches = 0.0;

    int footIndex = sanitized.indexOf(QLatin1Char('\''));

    if (footIndex >= 0) {

        QString footPart = sanitized.left(footIndex);

        if (!footPart.isEmpty()) {

            bool ok = false;

            feet = footPart.toDouble(&ok);

            if (!ok) {

                result.error = QObject::tr("invalid feet value");

                return result;

            }

        }

        sanitized = sanitized.mid(footIndex + 1);

    }

    int inchIndex = sanitized.indexOf(QLatin1Char('"'));

    if (inchIndex >= 0) {

        QString inchPart = sanitized.left(inchIndex);

        if (!inchPart.isEmpty()) {

            bool ok = false;

            inches = inchPart.toDouble(&ok);

            if (!ok) {

                result.error = QObject::tr("invalid inches value");

                return result;

            }

        }

        sanitized = sanitized.mid(inchIndex + 1);

    } else if (!sanitized.isEmpty()) {

        bool ok = false;

        if (footIndex >= 0) {

            inches = sanitized.toDouble(&ok);

        } else {

            feet = sanitized.toDouble(&ok);

        }

        if (!ok) {

            result.error = QObject::tr("invalid measurement");

            return result;

        }

        sanitized.clear();

    }

    if (!sanitized.isEmpty()) {

        result.error = QObject::tr("unexpected trailing characters");

        return result;

    }

    double totalMeters = (feet * 12.0 + inches) * 0.0254;

    if (negative)

        totalMeters = -totalMeters;

    result.ok = true;

    result.value = totalMeters;

    result.display = raw.trimmed();

    if (result.display.isEmpty()) {

        result.display = QObject::tr("%1 m").arg(totalMeters, 0, 'f', 3);

    }

    return result;

}

MeasurementParseResult parseMetricDistance(const QString& raw)

{

    MeasurementParseResult result;

    QString sanitized = removeWhitespace(raw.trimmed());

    if (sanitized.isEmpty()) {

        result.error = QObject::tr("empty measurement");

        return result;

    }

    QString lower = sanitized.toLower();

    double multiplier = 1.0;

    if (lower.endsWith(QStringLiteral("mm"))) {

        multiplier = 0.001;

        sanitized.chop(2);

    } else if (lower.endsWith(QStringLiteral("cm"))) {

        multiplier = 0.01;

        sanitized.chop(2);

    } else if (lower.endsWith(QStringLiteral("km"))) {

        multiplier = 1000.0;

        sanitized.chop(2);

    } else if (lower.endsWith(QStringLiteral("m"))) {

        multiplier = 1.0;

        sanitized.chop(1);

    }

    if (sanitized.isEmpty()) {

        result.error = QObject::tr("invalid measurement");

        return result;

    }

    bool ok = false;

    double magnitude = sanitized.toDouble(&ok);

    if (!ok) {

        result.error = QObject::tr("invalid measurement");

        return result;

    }

    result.ok = true;

    result.value = magnitude * multiplier;

    result.display = raw.trimmed();

    if (result.display.isEmpty()) {

        result.display = QObject::tr("%1 m").arg(result.value, 0, 'f', 3);

    }

    return result;

}

MeasurementParseResult parseDistanceMeasurement(const QString& raw, const QString& unitSystem)
{
    if (unitSystem == QLatin1String("imperial")) {
        return parseImperialDistance(raw);
    }
    return parseMetricDistance(raw);
}

MeasurementParseResult parseAngleMeasurement(const QString& raw)
{
    MeasurementParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty angle");
        return result;
    }

    QString lower = sanitized.toLower();
    double multiplier = kPi / 180.0;
    if (lower.endsWith(QStringLiteral("rad"))) {
        multiplier = 1.0;
        sanitized.chop(3);
    } else if (lower.endsWith(QStringLiteral("deg"))) {
        sanitized.chop(3);
    } else if (sanitized.endsWith(QChar(0x00B0))) {
        sanitized.chop(1);
    }

    if (sanitized.isEmpty()) {
        result.error = QObject::tr("invalid angle");
        return result;
    }

    bool ok = false;
    double magnitude = sanitized.toDouble(&ok);
    if (!ok) {
        result.error = QObject::tr("invalid angle");
        return result;
    }

    result.ok = true;
    result.value = magnitude * multiplier;
    result.display = raw.trimmed();
    if (result.display.isEmpty()) {
        result.display = QObject::tr("%1°").arg(magnitude, 0, 'f', 2);
    }
    return result;
}

MeasurementParseResult parseScaleMeasurement(const QString& raw)
{
    MeasurementParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty scale");
        return result;
    }

    bool percent = sanitized.endsWith(QLatin1Char('%'));
    if (percent) {
        sanitized.chop(1);
    }

    bool ok = false;
    double magnitude = sanitized.toDouble(&ok);
    if (!ok) {
        result.error = QObject::tr("invalid scale");
        return result;
    }

    if (percent) {
        magnitude /= 100.0;
    }

    if (magnitude <= 0.0) {
        result.error = QObject::tr("scale must be positive");
        return result;
    }

    result.ok = true;
    result.value = magnitude;
    result.display = raw.trimmed();
    if (percent && !result.display.endsWith(QLatin1Char('%'))) {
        result.display.append(QLatin1Char('%'));
    }
    if (result.display.isEmpty()) {
        result.display = QObject::tr("%1x").arg(magnitude, 0, 'f', 3);
    }
    return result;
}

MeasurementParseResult parseMeasurementValue(const QString& raw, const QString& unitSystem, Tool::MeasurementKind kind)

{

    switch (kind) {

    case Tool::MeasurementKind::Distance:

        return parseDistanceMeasurement(raw, unitSystem);

    case Tool::MeasurementKind::Angle:

        return parseAngleMeasurement(raw);

    case Tool::MeasurementKind::Scale:

        return parseScaleMeasurement(raw);

    default:

        break;

    }

    MeasurementParseResult result;

    result.error = QObject::tr("unsupported measurement");

    return result;

}

QString measurementHintForKind(Tool::MeasurementKind kind)

{

    switch (kind) {

    case Tool::MeasurementKind::Distance:

        return QObject::tr("Length (e.g. 12'6\" or 3.5m)");

    case Tool::MeasurementKind::Angle:

        return QObject::tr("Angle (e.g. 45° or 0.79rad)");

    case Tool::MeasurementKind::Scale:

        return QObject::tr("Scale (e.g. 2 or 150%)");

    default:

        return QObject::tr("Measurements");

    }

}

}

    QString MainWindow::navigationHintForTool(const QString& toolName) const

{

    QString base;

    if (toolName == QLatin1String("PanTool")) {

        base = tr("Pan: Drag to move the view target.");

    } else if (toolName == QLatin1String("OrbitTool")) {

        base = tr("Orbit: Drag to rotate around the target. Hold Shift while orbiting to pan.");

    } else if (toolName == QLatin1String("ZoomTool")) {

        base = tr("Zoom: Drag vertically to change distance. Hold Shift to invert drag direction. Ctrl-click zooms extents; Alt-click zooms the current selection.");

    } else {

        base = tr("Navigation tool");

    }

    if (!navigationPrefs)

        return base;

    const NavigationConfig& config = navigationPrefs->config();

    QStringList combos;

    auto buttonToText = [this](Qt::MouseButton button) -> QString {

        switch (button) {

        case Qt::LeftButton:

            return tr("Left mouse");

        case Qt::RightButton:

            return tr("Right mouse");

        case Qt::MiddleButton:

            return tr("Middle mouse");

        case Qt::ExtraButton1:

            return tr("Mouse button 4");

        case Qt::ExtraButton2:

            return tr("Mouse button 5");

        default:

            return QString();

        }

    };

    for (const auto& binding : config.dragBindings) {

        if (QString::fromStdString(binding.toolName) != toolName)

            continue;

        QString button = buttonToText(binding.button);

        if (button.isEmpty())

            continue;

        QStringList parts;

        if (binding.modifiers.testFlag(Qt::ShiftModifier))

            parts << tr("Shift");

        if (binding.modifiers.testFlag(Qt::ControlModifier))

            parts << tr("Ctrl");

        if (binding.modifiers.testFlag(Qt::AltModifier))

            parts << tr("Alt");

        parts << button;
    if (palettePrefs && !paletteActions.isEmpty()) {

        const QString activePalette = palettePrefs->activePaletteId();

        for (auto it = paletteActions.begin(); it != paletteActions.end(); ++it) {

            QAction* paletteAction = it.value();

            if (!paletteAction)

                continue;

            QSignalBlocker blocker(paletteAction);

            paletteAction->setChecked(it.key() == activePalette);

        }

    }

        QString combo = parts.join(QStringLiteral(" + "));

        if (!binding.temporary)

            combo = tr("%1 (toggle)").arg(combo);

        combos << combo;

    }

    if (toolName == QLatin1String("ZoomTool"))

        combos << tr("Scroll wheel");

    if (!combos.isEmpty())

        base += QStringLiteral(" ") + tr("Bindings: %1.").arg(combos.join(tr(", ")));

    if (toolName == QLatin1String("ZoomTool")) {

        base += QStringLiteral(" ");

        base += config.zoomToCursor ? tr("Zoom focuses on the cursor.") : tr("Zoom centers on the view target.");

        base += QStringLiteral(" ");

        base += tr("Zoom Extents (Ctrl+0) and Zoom Selection (Shift+Z) remain available.");

    }

    return base;

}

void MainWindow::refreshNavigationActionHints()

{

    if (panAction)

        panAction->setStatusTip(navigationHintForTool(QStringLiteral("PanTool")));

    if (orbitAction)

        orbitAction->setStatusTip(navigationHintForTool(QStringLiteral("OrbitTool")));

    if (zoomAction)

        zoomAction->setStatusTip(navigationHintForTool(QStringLiteral("ZoomTool")));

}

namespace {

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

    navigationPrefs = std::make_unique<NavigationPreferences>(this);

    toolManager = std::make_unique<ToolManager>(viewport->getDocument(), viewport->getCamera());

    toolManager->setNavigationConfig(navigationPrefs->config());

    viewport->setToolManager(toolManager.get());

    viewport->setNavigationPreferences(navigationPrefs.get());

    connect(viewport, &GLViewport::cursorPositionChanged, this, &MainWindow::updateCursor);

    connect(viewport, &GLViewport::frameStatsUpdated, this, &MainWindow::updateFrameStats);

    connect(navigationPrefs.get(), &NavigationPreferences::configChanged, this, [this]() {

        if (toolManager)

            toolManager->setNavigationConfig(navigationPrefs->config());

        refreshNavigationActionHints();

        if (hintLabel) {

            if (panAction && panAction->isChecked())

                hintLabel->setText(navigationHintForTool(QStringLiteral("PanTool")));

            else if (orbitAction && orbitAction->isChecked())

                hintLabel->setText(navigationHintForTool(QStringLiteral("OrbitTool")));

            else if (zoomAction && zoomAction->isChecked())

                hintLabel->setText(navigationHintForTool(QStringLiteral("ZoomTool")));

        }

    });

    // Load persisted theme and render style before applying styles

    QString storedViewPreset = QStringLiteral("iso");

    float storedFov = 60.0f;

    QString storedProjection = QStringLiteral("perspective");

    {

        QSettings settings("FreeCrafter", "FreeCrafter");

        darkTheme = settings.value(QStringLiteral("%1/darkTheme").arg(kSettingsGroup), true).toBool();

        renderStyleChoice = renderStyleFromSetting(settings.value(QStringLiteral("%1/renderStyle").arg(kSettingsGroup), QStringLiteral("shadedEdges")).toString());

        showHiddenGeometry = settings.value(QStringLiteral("%1/showHiddenGeometry").arg(kSettingsGroup), false).toBool();

        storedViewPreset = settings.value(QStringLiteral("%1/viewPreset").arg(kSettingsGroup), storedViewPreset).toString();

        storedFov = settings.value(QStringLiteral("%1/viewFov").arg(kSettingsGroup), storedFov).toFloat();

        storedProjection = settings.value(QStringLiteral("%1/viewProjection").arg(kSettingsGroup), storedProjection).toString();

        sunSettings.load(settings, QStringLiteral("Environment"));

    }

    viewport->setRenderStyle(renderStyleChoice);

    viewport->setShowHiddenGeometry(showHiddenGeometry);

    viewport->setSunSettings(sunSettings);

    viewport->setFieldOfView(storedFov);

    if (storedProjection.compare(QLatin1String("parallel"), Qt::CaseInsensitive) == 0)

        viewport->setProjectionMode(CameraController::ProjectionMode::Parallel);

    else

        viewport->setProjectionMode(CameraController::ProjectionMode::Perspective);

    if (!viewport->applyViewPreset(storedViewPreset))

        viewport->applyViewPreset(ViewPresetManager::StandardView::Iso);

    currentViewPresetId = viewport->currentViewPresetId();

    createMenus();

    createToolbars();
    createDockPanels();

    syncViewSettingsUI();

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
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
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

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    actionUndo = editMenu->addAction(tr("Undo"), this, &MainWindow::onUndo);
    actionUndo->setIcon(QIcon(QStringLiteral(":/icons/undo.svg")));
    actionRedo = editMenu->addAction(tr("Redo"), this, &MainWindow::onRedo);
    actionRedo->setIcon(QIcon(QStringLiteral(":/icons/redo.svg")));
    editMenu->addSeparator();
    actionPreferences = editMenu->addAction(tr("Preferences"), this, &MainWindow::showPreferences);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    actionZoomIn = viewMenu->addAction(tr("Zoom In"));
    actionZoomIn->setIcon(QIcon(QStringLiteral(":/icons/zoom_in.svg")));
    connect(actionZoomIn, &QAction::triggered, this, [this]() {
        if (!viewport)
            return;
        viewport->zoomInStep();
        statusBar()->showMessage(tr("Zoomed in"), 600);
    });

    actionZoomOut = viewMenu->addAction(tr("Zoom Out"));
    actionZoomOut->setIcon(QIcon(QStringLiteral(":/icons/zoom_out.svg")));
    connect(actionZoomOut, &QAction::triggered, this, [this]() {
        if (!viewport)
            return;
        viewport->zoomOutStep();
        statusBar()->showMessage(tr("Zoomed out"), 600);
    });

    actionZoomExtents = viewMenu->addAction(tr("Zoom Extents"));
    connect(actionZoomExtents, &QAction::triggered, this, [this]() {
        if (!viewport)
            return;
        if (viewport->zoomExtents()) {
            statusBar()->showMessage(tr("Zoomed to extents"), 1500);
        } else {
            statusBar()->showMessage(tr("No geometry to frame"), 1500);
        }
    });

    actionZoomSelection = viewMenu->addAction(tr("Zoom Selection"));
    actionZoomSelection->setIcon(QIcon(QStringLiteral(":/icons/zoom.png")));
    connect(actionZoomSelection, &QAction::triggered, this, [this]() {
        if (!viewport)
            return;
        if (viewport->zoomSelection()) {
            statusBar()->showMessage(tr("Zoomed to selection"), 1500);
        } else {
            statusBar()->showMessage(tr("Select objects to zoom"), 1500);
        }
    });

    viewMenu->addSeparator();

    actionViewIso = viewMenu->addAction(tr("Iso View"));
    actionViewIso->setShortcut(QKeySequence(QStringLiteral("Ctrl+1")));
    actionViewIso->setStatusTip(tr("Align the camera to an isometric three-quarter view"));
    connect(actionViewIso, &QAction::triggered, this, [this]() { applyStandardView(ViewPresetManager::StandardView::Iso); });

    actionViewTop = viewMenu->addAction(tr("Top View"));
    actionViewTop->setShortcut(QKeySequence(QStringLiteral("Ctrl+2")));
    actionViewTop->setStatusTip(tr("Look straight down from the top"));
    connect(actionViewTop, &QAction::triggered, this, [this]() { applyStandardView(ViewPresetManager::StandardView::Top); });

    actionViewBottom = viewMenu->addAction(tr("Bottom View"));
    actionViewBottom->setShortcut(QKeySequence(QStringLiteral("Ctrl+3")));
    actionViewBottom->setStatusTip(tr("Look straight up from beneath the model"));
    connect(actionViewBottom, &QAction::triggered, this, [this]() { applyStandardView(ViewPresetManager::StandardView::Bottom); });

    actionViewFront = viewMenu->addAction(tr("Front View"));
    actionViewFront->setShortcut(QKeySequence(QStringLiteral("Ctrl+4")));
    actionViewFront->setStatusTip(tr("Look directly at the front elevation"));
    connect(actionViewFront, &QAction::triggered, this, [this]() { applyStandardView(ViewPresetManager::StandardView::Front); });

    actionViewBack = viewMenu->addAction(tr("Back View"));
    actionViewBack->setShortcut(QKeySequence(QStringLiteral("Ctrl+5")));
    actionViewBack->setStatusTip(tr("Look directly at the back elevation"));
    connect(actionViewBack, &QAction::triggered, this, [this]() { applyStandardView(ViewPresetManager::StandardView::Back); });

    actionViewLeft = viewMenu->addAction(tr("Left View"));
    actionViewLeft->setShortcut(QKeySequence(QStringLiteral("Ctrl+6")));
    actionViewLeft->setStatusTip(tr("Look from the model's left side"));
    connect(actionViewLeft, &QAction::triggered, this, [this]() { applyStandardView(ViewPresetManager::StandardView::Left); });

    actionViewRight = viewMenu->addAction(tr("Right View"));
    actionViewRight->setShortcut(QKeySequence(QStringLiteral("Ctrl+7")));
    actionViewRight->setStatusTip(tr("Look from the model's right side"));
    connect(actionViewRight, &QAction::triggered, this, [this]() { applyStandardView(ViewPresetManager::StandardView::Right); });

    viewMenu->addSeparator();

    actionToggleProjection = viewMenu->addAction(tr("Parallel Projection"));
    actionToggleProjection->setCheckable(true);
    actionToggleProjection->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    actionToggleProjection->setStatusTip(tr("Toggle between perspective and parallel projections"));
    if (viewport)
        actionToggleProjection->setChecked(viewport->projectionMode() == CameraController::ProjectionMode::Parallel);
    connect(actionToggleProjection, &QAction::toggled, this, [this](bool checked) {
        setProjectionMode(checked ? CameraController::ProjectionMode::Parallel : CameraController::ProjectionMode::Perspective);
    });

    const bool gridInitiallyVisible = viewport ? viewport->isGridVisible() : true;
    gridAction = viewMenu->addAction(QIcon(QStringLiteral(":/icons/grid.svg")), tr("Show Grid"));
    gridAction->setCheckable(true);
    gridAction->setChecked(gridInitiallyVisible);
    gridAction->setStatusTip(tr("Show or hide the ground grid"));
    connect(gridAction, &QAction::toggled, this, &MainWindow::toggleGrid);

    actionViewSettings = viewMenu->addAction(tr("Camera Settings…"), this, &MainWindow::showViewSettingsDialog);
    actionViewSettings->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    actionViewSettings->setStatusTip(tr("Adjust field of view and projection settings"));

    QAction* actionSplitView = viewMenu->addAction(tr("Split View"), this, [this]() {
        statusBar()->showMessage(tr("Split view is coming soon"), 2000);
    });
    actionSplitView->setEnabled(false);

    actionToggleRightDock = viewMenu->addAction(tr("Toggle Right Panel"), this, &MainWindow::toggleRightDock);
    actionToggleTheme = viewMenu->addAction(tr("Toggle Theme"), this, &MainWindow::toggleTheme);

    renderStyleGroup = new QActionGroup(this);
    renderStyleGroup->setExclusive(true);
    QMenu* renderStyleMenu = viewMenu->addMenu(tr("Render Style"));

    renderWireframeAction = renderStyleMenu->addAction(tr("Wireframe"));
    renderWireframeAction->setCheckable(true);
    renderWireframeAction->setStatusTip(tr("Display geometry as wireframe"));
    renderStyleGroup->addAction(renderWireframeAction);

    renderHiddenLineAction = renderStyleMenu->addAction(tr("Hidden Line"));
    renderHiddenLineAction->setCheckable(true);
    renderHiddenLineAction->setStatusTip(tr("Display visible edges with dashed hidden lines"));
    renderStyleGroup->addAction(renderHiddenLineAction);

    renderShadedAction = renderStyleMenu->addAction(tr("Shaded"));
    renderShadedAction->setCheckable(true);
    renderShadedAction->setStatusTip(tr("Display solid shading without edge overlays"));
    renderStyleGroup->addAction(renderShadedAction);

    renderShadedEdgesAction = renderStyleMenu->addAction(tr("Shaded + Edges"));
    renderShadedEdgesAction->setCheckable(true);
    renderShadedEdgesAction->setStatusTip(tr("Display shading with edge overlays"));
    renderStyleGroup->addAction(renderShadedEdgesAction);

    renderMonochromeAction = renderStyleMenu->addAction(tr("Monochrome"));
    renderMonochromeAction->setCheckable(true);
    renderMonochromeAction->setStatusTip(tr("Display flat gray shading with edge outlines"));
    renderStyleGroup->addAction(renderMonochromeAction);

    connect(renderWireframeAction, &QAction::triggered, this, [this]() { setRenderStyle(Renderer::RenderStyle::Wireframe); });
    connect(renderHiddenLineAction, &QAction::triggered, this, [this]() { setRenderStyle(Renderer::RenderStyle::HiddenLine); });
    connect(renderShadedAction, &QAction::triggered, this, [this]() { setRenderStyle(Renderer::RenderStyle::Shaded); });
    connect(renderShadedEdgesAction, &QAction::triggered, this, [this]() { setRenderStyle(Renderer::RenderStyle::ShadedWithEdges); });
    connect(renderMonochromeAction, &QAction::triggered, this, [this]() { setRenderStyle(Renderer::RenderStyle::Monochrome); });

    renderWireframeAction->setChecked(renderStyleChoice == Renderer::RenderStyle::Wireframe);
    renderShadedAction->setChecked(renderStyleChoice == Renderer::RenderStyle::Shaded);
    renderShadedEdgesAction->setChecked(renderStyleChoice == Renderer::RenderStyle::ShadedWithEdges);
    renderHiddenLineAction->setChecked(renderStyleChoice == Renderer::RenderStyle::HiddenLine);
    renderMonochromeAction->setChecked(renderStyleChoice == Renderer::RenderStyle::Monochrome);

    viewMenu->addSeparator();
    actionViewHiddenGeometry = viewMenu->addAction(tr("View Hidden Geometry"));
    actionViewHiddenGeometry->setCheckable(true);
    actionViewHiddenGeometry->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+H")));
    actionViewHiddenGeometry->setStatusTip(tr("Toggle visibility of objects marked as hidden"));
    actionViewHiddenGeometry->setChecked(showHiddenGeometry);
    connect(actionViewHiddenGeometry, &QAction::toggled, this, [this](bool checked) {
        showHiddenGeometry = checked;
        if (viewport) {
            viewport->setShowHiddenGeometry(checked);
        }
        if (statusBar()) {
            statusBar()->showMessage(checked ? tr("Hidden geometry shown") : tr("Hidden geometry hidden"), 2000);
        }
        persistViewSettings();
    });

    QMenu* insertMenu = menuBar()->addMenu(tr("&Insert"));
    insertMenu->addAction(tr("Shapes"), this, [this]() { statusBar()->showMessage(tr("Insert Shapes not implemented"), 2000); });
    insertMenu->addAction(tr("Guides"), this, [this]() { statusBar()->showMessage(tr("Insert Guides not implemented"), 2000); });
    insertMenu->addAction(tr("Images"), this, [this]() { statusBar()->showMessage(tr("Insert Images not implemented"), 2000); });
    insertMenu->addAction(tr("External Reference"), this, [this]() {
        statusBar()->showMessage(tr("External references not implemented"), 2000);
    });

    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Plugins…"), this, [this]() { statusBar()->showMessage(tr("Plugin manager not implemented"), 2000); });
    actionPalette = toolsMenu->addAction(tr("Command Palette…"), this, &MainWindow::showCommandPalette);
    actionPalette->setIcon(QIcon(QStringLiteral(":/icons/search.svg")));

    QMenu* windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(tr("New Window"), this, [this]() { statusBar()->showMessage(tr("New window not implemented"), 2000); });

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    actionShortcuts = helpMenu->addAction(tr("Keyboard Shortcuts"), this, &MainWindow::showKeyboardShortcuts);
    actionShortcuts->setIcon(QIcon(QStringLiteral(":/icons/help.svg")));
    helpMenu->addAction(tr("About"), this, [this]() {
        QMessageBox::about(this, tr("About FreeCrafter"), tr("FreeCrafter — MIT Licensed CAD shell prototype."));
    });
}

void MainWindow::importExternalModel()

{

    const QString filter = tr("3D Models (*.obj *.stl *.fbx *.dxf *.dwg);;Wavefront OBJ (*.obj);;STL (*.stl);;FBX (*.fbx);;DXF (*.dxf);;DWG (*.dwg)");

    const QString fn = QFileDialog::getOpenFileName(this, tr("Import Model"), QString(), filter);

    if (fn.isEmpty())

        return;

    Scene::Document::FileFormat format = Scene::Document::FileFormat::Auto;

    const QString suffix = QFileInfo(fn).suffix().toLower();

    if (suffix == QLatin1String("obj")) {

        format = Scene::Document::FileFormat::Obj;

    } else if (suffix == QLatin1String("stl")) {

        format = Scene::Document::FileFormat::Stl;

    } else if (suffix == QLatin1String("fbx")) {

        format = Scene::Document::FileFormat::Fbx;

    } else if (suffix == QLatin1String("dxf")) {

        format = Scene::Document::FileFormat::Dxf;

    } else if (suffix == QLatin1String("dwg")) {

        format = Scene::Document::FileFormat::Dwg;

    }

    Scene::Document* doc = viewport ? viewport->getDocument() : nullptr;

    if (!doc) {

        statusBar()->showMessage(tr("No active document for import"), 2000);

        return;

    }

    bool ok = doc->importExternalModel(fn.toStdString(), format);

    if (ok) {

        statusBar()->showMessage(tr("Imported %1").arg(QFileInfo(fn).fileName()), 1500);

        viewport->update();

    } else {

        QString reason = QString::fromStdString(doc->lastImportError());

        if (reason.isEmpty())

            reason = tr("Unknown import error");

        statusBar()->showMessage(tr("Failed to import %1: %2").arg(QFileInfo(fn).fileName(), reason), 4000);

    }

}

void MainWindow::exportFile()

{

    if (!viewport) {
        statusBar()->showMessage(tr("No active viewport to export"), 4000);
        return;
    }

    Scene::Document* document = viewport->getDocument();
    if (!document) {
        statusBar()->showMessage(tr("No document attached to viewport"), 4000);
        return;
    }

    auto formats = FileIO::Exporters::supportedFormats();
    if (formats.empty()) {
        const QString message = tr("Export workflow not implemented in this build");
        QMessageBox::information(this, tr("Export Unavailable"), message);
        statusBar()->showMessage(message, 2000);
        return;
    }

    QStringList filters;
    for (auto format : formats) {
        filters << QString::fromStdString(FileIO::formatFilterString(format));
    }

    QFileDialog dialog(this, tr("Export Model"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilters(filters);
    if (!filters.isEmpty()) {
        dialog.selectNameFilter(filters.first());
        const auto& defaultFormat = formats.front();
        const std::string extension = FileIO::formatExtension(defaultFormat);
        if (!extension.empty()) {
            dialog.setDefaultSuffix(QString::fromStdString(extension.substr(1)));
        }
    }

    if (dialog.exec() == QDialog::Rejected) {
        return;
    }

    const QStringList selected = dialog.selectedFiles();
    if (selected.isEmpty()) {
        return;
    }

    QString filePath = selected.first();
    QString selectedFilter = dialog.selectedNameFilter();
    FileIO::SceneFormat chosenFormat = formats.front();
    for (auto format : formats) {
        if (selectedFilter == QString::fromStdString(FileIO::formatFilterString(format))) {
            chosenFormat = format;
            break;
        }
    }

    QString extension = QString::fromStdString(FileIO::formatExtension(chosenFormat));
    if (!extension.isEmpty() && !filePath.endsWith(extension, Qt::CaseInsensitive)) {
        filePath += extension;
    }

    document->synchronizeWithGeometry();

    std::string errorMessage;
    if (!FileIO::Exporters::exportScene(*document, filePath.toStdString(), chosenFormat, &errorMessage)) {
        QString message = QString::fromStdString(errorMessage);
        if (message.isEmpty()) {
            message = tr("Unknown export error");
        }
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Could not export the scene:\n%1").arg(message));
        statusBar()->showMessage(tr("Export failed"), 5000);
        return;
    }

    statusBar()->showMessage(tr("Exported \"%1\"").arg(QFileInfo(filePath).fileName()), 4000);
}


void MainWindow::createToolbars()

{

    primaryToolbar = addToolBar(tr("Primary"));

    primaryToolbar->setMovable(false);

    primaryToolbar->setFixedHeight(kToolbarHeight);

    primaryToolbar->setIconSize(QSize(24, 24));

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

    primaryToolbar->addSeparator();

    renderStyleButton = new QToolButton(primaryToolbar);

    renderStyleButton->setPopupMode(QToolButton::InstantPopup);

    renderStyleButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

    renderStyleButton->setMenu(new QMenu(renderStyleButton));

    renderStyleButton->menu()->addAction(renderWireframeAction);

    renderStyleButton->menu()->addAction(renderHiddenLineAction);

    renderStyleButton->menu()->addAction(renderShadedAction);

    renderStyleButton->menu()->addAction(renderShadedEdgesAction);

    renderStyleButton->menu()->addAction(renderMonochromeAction);

    renderStyleButton->setToolTip(tr("Select render style (Wireframe, Hidden Line, Shaded, Shaded + Edges, Monochrome)"));

    auto* renderWidgetAction = new QWidgetAction(primaryToolbar);

    renderWidgetAction->setDefaultWidget(renderStyleButton);

    primaryToolbar->addAction(renderWidgetAction);

    viewPresetToolButton = new QToolButton(primaryToolbar);

    viewPresetToolButton->setPopupMode(QToolButton::InstantPopup);

    viewPresetToolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

    QMenu* viewPresetMenu = new QMenu(viewPresetToolButton);

    viewPresetMenu->addAction(actionViewIso);

    viewPresetMenu->addAction(actionViewTop);

    viewPresetMenu->addAction(actionViewBottom);

    viewPresetMenu->addAction(actionViewFront);

    viewPresetMenu->addAction(actionViewBack);

    viewPresetMenu->addAction(actionViewLeft);

    viewPresetMenu->addAction(actionViewRight);

    viewPresetMenu->addSeparator();

    viewPresetMenu->addAction(actionToggleProjection);

    viewPresetMenu->addAction(actionViewSettings);

    viewPresetToolButton->setMenu(viewPresetMenu);

    viewPresetToolButton->setToolTip(tr("Select standard views and projection options"));

    auto* viewPresetWidgetAction = new QWidgetAction(primaryToolbar);

    viewPresetWidgetAction->setDefaultWidget(viewPresetToolButton);

    primaryToolbar->addAction(viewPresetWidgetAction);

    updateViewPresetButtonLabel();

    toolRibbon = new QToolBar(tr("Tools"), this);

    toolRibbon->setOrientation(Qt::Vertical);

    toolRibbon->setMovable(false);

    toolRibbon->setIconSize(QSize(24, 24));

    toolRibbon->setFixedWidth(kRibbonWidth);

    toolRibbon->setToolButtonStyle(Qt::ToolButtonIconOnly);

    toolActionGroup = new QActionGroup(this);

    toolActionGroup->setExclusive(true);

    selectAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/select.png")), tr("Select"), this, &MainWindow::activateSelect);

    selectAction->setCheckable(true);

    selectAction->setStatusTip(tr("Select and edit existing geometry."));

    toolActionGroup->addAction(selectAction);

    lineAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/line.png")), tr("Line"), this, &MainWindow::activateLine);

    lineAction->setCheckable(true);

    lineAction->setStatusTip(tr("Draw connected line segments."));

    toolActionGroup->addAction(lineAction);

    rectangleAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/rectangle.png")), tr("Rectangle"), this, &MainWindow::activateRectangle);

    rectangleAction->setCheckable(true);

    rectangleAction->setStatusTip(tr("Create rectangles aligned to the axes."));

    toolActionGroup->addAction(rectangleAction);

    circleAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/circle.png")), tr("Circle"), this, &MainWindow::activateCircle);

    circleAction->setCheckable(true);

    circleAction->setStatusTip(tr("Draw circles by center and radius."));

    toolActionGroup->addAction(circleAction);

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

    sectionAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/section.png")), tr("Section"), this, &MainWindow::activateSection);

    sectionAction->setCheckable(true);

    toolActionGroup->addAction(sectionAction);

    toolRibbon->addSeparator();

    panAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/pan.svg")), tr("Pan"), this, &MainWindow::activatePan);

    panAction->setCheckable(true);

    toolActionGroup->addAction(panAction);

    orbitAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/orbit.svg")), tr("Orbit"), this, &MainWindow::activateOrbit);

    orbitAction->setCheckable(true);

    toolActionGroup->addAction(orbitAction);

    zoomAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/zoom.png")), tr("Zoom"), this, &MainWindow::activateZoom);

    zoomAction->setCheckable(true);

    toolActionGroup->addAction(zoomAction);

    measureAction = toolRibbon->addAction(QIcon(QStringLiteral(":/icons/measure.svg")), tr("Measure"), this, &MainWindow::activateMeasure);

    measureAction->setCheckable(true);

    toolActionGroup->addAction(measureAction);

    if (gridAction) {

        gridAction->setIcon(QIcon(QStringLiteral(":/icons/grid.svg")));

        gridAction->setToolTip(tr("Toggle Grid"));

        toolRibbon->addAction(gridAction);

    }

    addToolBar(Qt::LeftToolBarArea, toolRibbon);

    selectAction->setChecked(true);

    activateSelect();

    setRenderStyle(renderStyleChoice);

    refreshNavigationActionHints();

}

void MainWindow::createDockPanels()
{
    rightDock = new QDockWidget(tr("Panels"), this);
    rightDock->setObjectName(QStringLiteral("PanelsDock"));
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

    environmentPanel = new EnvironmentPanel(this);
    environmentPanel->setSettings(sunSettings);
    connect(environmentPanel, &EnvironmentPanel::settingsChanged, this, &MainWindow::handleSunSettingsChanged);

    environmentDock = new QDockWidget(tr("Environment"), this);
    environmentDock->setObjectName(QStringLiteral("EnvironmentDock"));
    environmentDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    environmentDock->setMinimumWidth(260);
    environmentDock->setMaximumWidth(420);
    environmentDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    environmentDock->setWidget(environmentPanel);

    addDockWidget(Qt::RightDockWidgetArea, environmentDock);
    splitDockWidget(rightDock, environmentDock, Qt::Vertical);
}
void MainWindow::createStatusBarWidgets()

{

    statusBar()->setSizeGripEnabled(true);

    statusBar()->setMinimumHeight(kStatusHeight);

    coordLabel = new QLabel(tr("X: --  Y: --  Z: --"), this);

    selectionLabel = new QLabel(tr("Selection: 0 items"), this);

    hintLabel = new QLabel(tr("Ready"), this);

    frameLabel = new QLabel(tr("Frame: -- ms • -- draws • -- fps"), this);

    taskLabel = new QLabel(tr("No background tasks"), this);

    measurementWidget = new MeasurementWidget(this);

    connect(measurementWidget, &MeasurementWidget::measurementCommitted, this, &MainWindow::handleMeasurementCommit);

    statusBar()->addWidget(coordLabel);

    statusBar()->addWidget(selectionLabel);

    statusBar()->addPermanentWidget(hintLabel, 1);

    statusBar()->addPermanentWidget(measurementWidget, 0);

    statusBar()->addPermanentWidget(frameLabel, 0);

    statusBar()->addPermanentWidget(taskLabel, 0);

    updateSelectionStatus();

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

    hotkeys.registerAction(QStringLiteral("view.zoomSelection"), actionZoomSelection);

    hotkeys.registerAction(QStringLiteral("view.iso"), actionViewIso);

    hotkeys.registerAction(QStringLiteral("view.top"), actionViewTop);

    hotkeys.registerAction(QStringLiteral("view.bottom"), actionViewBottom);

    hotkeys.registerAction(QStringLiteral("view.front"), actionViewFront);

    hotkeys.registerAction(QStringLiteral("view.back"), actionViewBack);

    hotkeys.registerAction(QStringLiteral("view.left"), actionViewLeft);

    hotkeys.registerAction(QStringLiteral("view.right"), actionViewRight);

    hotkeys.registerAction(QStringLiteral("view.toggleProjection"), actionToggleProjection);

    hotkeys.registerAction(QStringLiteral("view.cameraSettings"), actionViewSettings);

    hotkeys.registerAction(QStringLiteral("view.toggleRightDock"), actionToggleRightDock);

    hotkeys.registerAction(QStringLiteral("theme.toggle"), actionToggleTheme);

    hotkeys.registerAction(QStringLiteral("tools.select"), selectAction);

    hotkeys.registerAction(QStringLiteral("tools.line"), lineAction);
    hotkeys.registerAction(QStringLiteral("tools.rectangle"), rectangleAction);
    hotkeys.registerAction(QStringLiteral("tools.circle"), circleAction);

    hotkeys.registerAction(QStringLiteral("tools.move"), moveAction);

    hotkeys.registerAction(QStringLiteral("tools.rotate"), rotateAction);

    hotkeys.registerAction(QStringLiteral("tools.scale"), scaleAction);

    hotkeys.registerAction(QStringLiteral("tools.extrude"), extrudeAction);

    hotkeys.registerAction(QStringLiteral("tools.section"), sectionAction);

    hotkeys.registerAction(QStringLiteral("tools.pan"), panAction);

    hotkeys.registerAction(QStringLiteral("tools.orbit"), orbitAction);

    hotkeys.registerAction(QStringLiteral("tools.zoom"), zoomAction);

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
        if (rectangleAction) rectangleAction->setToolTip(format(rectangleAction, tr("Rectangle")));
        if (circleAction) circleAction->setToolTip(format(circleAction, tr("Circle")));

        if (moveAction) moveAction->setToolTip(format(moveAction, tr("Move")));

        if (rotateAction) rotateAction->setToolTip(format(rotateAction, tr("Rotate")));

        if (scaleAction) scaleAction->setToolTip(format(scaleAction, tr("Scale")));

        if (panAction) panAction->setToolTip(format(panAction, tr("Pan")));

        if (orbitAction) orbitAction->setToolTip(format(orbitAction, tr("Orbit")));

        if (zoomAction) zoomAction->setToolTip(format(zoomAction, tr("Zoom")));

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

    sunSettings.save(settings, QStringLiteral("Environment"));

    persistViewSettings();

}

void MainWindow::setActiveTool(QAction* action, const QString& toolId, const QString& hint)

{

    if (!action)

        return;

    if (!toolManager)

        return;

    Tool::MeasurementKind kind = Tool::MeasurementKind::None;

    if (!toolId.isEmpty()) {

        toolManager->activateTool(toolId.toUtf8().constData(), false);

        kind = toolManager->getMeasurementKind();

    }

    action->setChecked(true);

    if (hintLabel)

        hintLabel->setText(hint);

    if (measurementWidget)

        measurementWidget->setHint(measurementHintForKind(kind));

}

void MainWindow::updateThemeActionIcon()

{

    if (!actionToggleTheme)

        return;

    const QString iconPath = darkTheme ? QStringLiteral(":/icons/theme_light.svg") : QStringLiteral(":/icons/theme_dark.svg");

    actionToggleTheme->setIcon(QIcon(iconPath));

}

void MainWindow::setRenderStyle(Renderer::RenderStyle style)

{

    bool changed = (renderStyleChoice != style);

    renderStyleChoice = style;

    if (viewport) {

        viewport->setRenderStyle(style);

    }

    if (renderWireframeAction) {

        renderWireframeAction->setChecked(style == Renderer::RenderStyle::Wireframe);

    }

    if (renderHiddenLineAction) {

        renderHiddenLineAction->setChecked(style == Renderer::RenderStyle::HiddenLine);

    }

    if (renderShadedAction) {

        renderShadedAction->setChecked(style == Renderer::RenderStyle::Shaded);

    }

    if (renderShadedEdgesAction) {

        renderShadedEdgesAction->setChecked(style == Renderer::RenderStyle::ShadedWithEdges);

    }

    if (renderMonochromeAction) {

        renderMonochromeAction->setChecked(style == Renderer::RenderStyle::Monochrome);

    }

    if (renderStyleButton) {

        QString label;

        switch (style) {

        case Renderer::RenderStyle::Wireframe:

            label = tr("Style: Wireframe");

            break;

        case Renderer::RenderStyle::HiddenLine:

            label = tr("Style: Hidden Line");

            break;

        case Renderer::RenderStyle::Shaded:

            label = tr("Style: Shaded");

            break;

        case Renderer::RenderStyle::ShadedWithEdges:

            label = tr("Style: Shaded+Edges");

            break;

        case Renderer::RenderStyle::Monochrome:

            label = tr("Style: Monochrome");

            break;

        default:

            label = tr("Style: Shaded+Edges");

            break;

        }

        renderStyleButton->setText(label);

    }

    if (changed) {

        QSettings settings("FreeCrafter", "FreeCrafter");

        settings.setValue(QStringLiteral("%1/renderStyle").arg(kSettingsGroup), renderStyleToSetting(style));

    }

}

void MainWindow::applyStandardView(ViewPresetManager::StandardView view)

{

    if (!viewport)

        return;

    if (!viewport->applyViewPreset(view))

        return;

    if (const auto* preset = viewPresetManager.preset(view))

        currentViewPresetId = preset->id;

    else

        currentViewPresetId = viewport->currentViewPresetId();

    syncViewSettingsUI();

    const QString label = viewPresetManager.labelForId(currentViewPresetId);

    if (!label.isEmpty())

        statusBar()->showMessage(tr("Switched to %1 view").arg(label), 1500);

    persistViewSettings();

}

void MainWindow::setProjectionMode(CameraController::ProjectionMode mode, bool showStatus)

{

    if (!viewport)

        return;

    viewport->setProjectionMode(mode);

    if (actionToggleProjection) {

        QSignalBlocker blocker(actionToggleProjection);

        actionToggleProjection->setChecked(mode == CameraController::ProjectionMode::Parallel);

    }

    if (showStatus) {

        statusBar()->showMessage(mode == CameraController::ProjectionMode::Parallel ? tr("Projection: Parallel")

                                                                                : tr("Projection: Perspective"),

                                 1500);

    }

    updateViewPresetButtonLabel();

    persistViewSettings();

}

void MainWindow::updateViewPresetButtonLabel()

{

    if (!viewPresetToolButton)

        return;

    QString label = viewPresetManager.labelForId(currentViewPresetId);

    if (label.isEmpty())

        label = tr("Custom");

    QString projectionText = viewport && viewport->projectionMode() == CameraController::ProjectionMode::Parallel

                                 ? tr("Ortho")

                                 : tr("Persp");

    viewPresetToolButton->setText(tr("%1 • %2").arg(label, projectionText));

    viewPresetToolButton->setToolTip(tr("%1 view (%2 projection)").arg(label, projectionText));

}

void MainWindow::persistViewSettings() const

{

    if (!viewport)

        return;

    QSettings settings("FreeCrafter", "FreeCrafter");

    settings.beginGroup(kSettingsGroup);

    settings.setValue(QStringLiteral("viewPreset"), currentViewPresetId);

    settings.setValue(QStringLiteral("viewFov"), viewport->fieldOfView());

    settings.setValue(QStringLiteral("viewProjection"),

                      viewport->projectionMode() == CameraController::ProjectionMode::Parallel ? QStringLiteral("parallel")

                                                                                              : QStringLiteral("perspective"));

    settings.setValue(QStringLiteral("renderStyle"), renderStyleToSetting(renderStyleChoice));

    settings.setValue(QStringLiteral("showHiddenGeometry"), viewport->isHiddenGeometryVisible());

    settings.endGroup();

}

void MainWindow::syncViewSettingsUI()

{

    if (actionToggleProjection && viewport) {

        QSignalBlocker blocker(actionToggleProjection);

        actionToggleProjection->setChecked(viewport->projectionMode() == CameraController::ProjectionMode::Parallel);

    }

    if (actionViewHiddenGeometry && viewport) {

        QSignalBlocker blocker(actionViewHiddenGeometry);

        actionViewHiddenGeometry->setChecked(viewport->isHiddenGeometryVisible());

    }

    updateViewPresetButtonLabel();

}

void MainWindow::handleSunSettingsChanged(const SunSettings& settings)

{

    SunSettings previous = sunSettings;

    sunSettings = settings;

    if (viewport)

        viewport->setSunSettings(sunSettings);

    QSettings settingsStore("FreeCrafter", "FreeCrafter");

    sunSettings.save(settingsStore, QStringLiteral("Environment"));

    updateShadowStatus(previous, sunSettings);

}

void MainWindow::updateShadowStatus(const SunSettings& previous, const SunSettings& current)

{

    if (!statusBar())

        return;

    bool changed = previous.shadowsEnabled != current.shadowsEnabled

        || previous.date != current.date

        || previous.time != current.time

        || !qFuzzyCompare(previous.latitude, current.latitude)

        || !qFuzzyCompare(previous.longitude, current.longitude)

        || previous.timezoneMinutes != current.timezoneMinutes

        || previous.daylightSaving != current.daylightSaving;

    if (!changed)

        return;

    if (!current.shadowsEnabled) {

        statusBar()->showMessage(tr("Shadows disabled"), 2000);

        return;

    }

    SunModel::Result result = SunModel::computeSunDirection(current.date,

                                                            current.time,

                                                            current.latitude,

                                                            current.longitude,

                                                            current.effectiveTimezoneMinutes());

    if (result.valid) {

        statusBar()->showMessage(tr("Shadows enabled — Sun altitude %1°, azimuth %2°")

                                     .arg(result.altitudeDegrees, 0, 'f', 1)

                                     .arg(result.azimuthDegrees, 0, 'f', 1),

                                 4000);

    } else {

        statusBar()->showMessage(tr("Shadows enabled — sun below horizon"), 4000);

    }

}

void MainWindow::newFile()

{

    if (auto* doc = viewport->getDocument()) {

        doc->reset();

    }

    viewport->update();

    statusBar()->showMessage(tr("New document created"), 1500);

}

void MainWindow::openFile()

{

    const QString fn = QFileDialog::getOpenFileName(this, tr("Open FreeCrafter Model"), QString(), tr("FreeCrafter (*.fcm)"));

    if (fn.isEmpty()) return;

    bool ok = viewport->getDocument() && viewport->getDocument()->loadFromFile(fn.toStdString());

    viewport->update();

    if (ok) {

        statusBar()->showMessage(tr("Opened %1").arg(QFileInfo(fn).fileName()), 1500);

    } else {

        statusBar()->showMessage(tr("Failed to open %1").arg(QFileInfo(fn).fileName()), 2000);

    }

}

void MainWindow::saveFile()

{

    const QString fn = QFileDialog::getSaveFileName(this, tr("Save FreeCrafter Model"), QString(), tr("FreeCrafter (*.fcm)"));

    if (fn.isEmpty()) return;

    bool ok = viewport->getDocument() && viewport->getDocument()->saveToFile(fn.toStdString());

    if (ok) {

        statusBar()->showMessage(tr("Saved %1").arg(QFileInfo(fn).fileName()), 1500);

    } else {

        statusBar()->showMessage(tr("Failed to save %1").arg(QFileInfo(fn).fileName()), 2000);

    }

}

void MainWindow::saveFileAs()

{

    saveFile();

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

    QDialog dialog(this);

    dialog.setWindowTitle(tr("Preferences"));

    dialog.resize(440, 360);

    auto* layout = new QVBoxLayout(&dialog);

    auto* navLabel = new QLabel(tr("Navigation style"), &dialog);

    navLabel->setWordWrap(true);

    layout->addWidget(navLabel);

    auto* schemeCombo = new QComboBox(&dialog);

    QList<NavigationPreferences::SchemeInfo> schemes = navigationPrefs ? navigationPrefs->availableSchemes() : QList<NavigationPreferences::SchemeInfo>{};

    int activeIndex = 0;

    QString activeSchemeId = navigationPrefs ? navigationPrefs->activeScheme() : QString();

    for (int i = 0; i < schemes.size(); ++i) {

        schemeCombo->addItem(schemes[i].label, schemes[i].id);

        if (schemes[i].id == activeSchemeId)

            activeIndex = i;

    }

    layout->addWidget(schemeCombo);

    auto* schemeDescription = new QLabel(&dialog);

    schemeDescription->setWordWrap(true);

    layout->addWidget(schemeDescription);

    auto* cursorCheck = new QCheckBox(tr("Zoom towards cursor"), &dialog);

    layout->addWidget(cursorCheck);

    auto* invertCheck = new QCheckBox(tr("Invert scroll direction"), &dialog);

    layout->addWidget(invertCheck);

    auto* shortcutsButton = new QPushButton(tr("Customize Shortcuts…"), &dialog);

    layout->addWidget(shortcutsButton);

    connect(shortcutsButton, &QPushButton::clicked, this, [this, &dialog]() {

        hotkeys.showEditor(&dialog);

    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto updateControls = [&](int index, bool useStored) {

        if (index < 0 || index >= schemes.size())

            return;

        const auto& info = schemes[index];

        schemeDescription->setText(info.description);

        if (navigationPrefs && useStored && info.id == navigationPrefs->activeScheme()) {

            cursorCheck->setChecked(navigationPrefs->config().zoomToCursor);

            invertCheck->setChecked(navigationPrefs->config().invertWheel);

        } else if (navigationPrefs) {

            NavigationConfig preview = navigationPrefs->schemeConfig(info.id);

            cursorCheck->setChecked(preview.zoomToCursor);

            invertCheck->setChecked(preview.invertWheel);

        } else {

            cursorCheck->setChecked(true);

            invertCheck->setChecked(false);

        }

    };

    schemeCombo->blockSignals(true);

    schemeCombo->setCurrentIndex(activeIndex);

    schemeCombo->blockSignals(false);

    updateControls(activeIndex, true);

    connect(schemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, [=, &updateControls](int index) {

        updateControls(index, false);

    });

    if (dialog.exec() != QDialog::Accepted)

        return;

    if (!navigationPrefs)

        return;

    const QString chosenScheme = schemeCombo->currentData().toString();

    navigationPrefs->setActiveScheme(chosenScheme);

    navigationPrefs->setZoomToCursor(cursorCheck->isChecked());

    navigationPrefs->setInvertWheel(invertCheck->isChecked());

    navigationPrefs->save();

}

void MainWindow::showCommandPalette()

{

    statusBar()->showMessage(tr("Command palette coming soon"), 2000);

}

void MainWindow::toggleRightDock()
{
    const bool anyVisible = (rightDock && rightDock->isVisible()) || (environmentDock && environmentDock->isVisible());
    const bool target = !anyVisible;

    if (rightDock)
        rightDock->setVisible(target);
    if (environmentDock)
        environmentDock->setVisible(target);
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

void MainWindow::activateRectangle()
{
    setActiveTool(rectangleAction, QStringLiteral("Rectangle"),
        tr("Rectangle: Click the first corner, then click the opposite corner to finish."));
}

void MainWindow::activateCircle()
{
    setActiveTool(circleAction, QStringLiteral("Circle"),
        tr("Circle: Click to set the center, then click again to define the radius."));
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

void MainWindow::activateSection()

{

    setActiveTool(sectionAction,

                  QStringLiteral("SectionTool"),

                  tr("Section Plane: Click to place a cutting plane using current inference cues."));

}

void MainWindow::activatePan()

{

    setActiveTool(panAction, QStringLiteral("PanTool"), navigationHintForTool(QStringLiteral("PanTool")));

}

void MainWindow::activateOrbit()

{

    setActiveTool(orbitAction, QStringLiteral("OrbitTool"), navigationHintForTool(QStringLiteral("OrbitTool")));

}

void MainWindow::activateZoom()

{

    setActiveTool(zoomAction, QStringLiteral("ZoomTool"), navigationHintForTool(QStringLiteral("ZoomTool")));

}

void MainWindow::activateMeasure()

{

    setActiveTool(measureAction, QString(), tr("Measure: Click two points to sample distance"));

}

void MainWindow::toggleGrid(bool enabled)

{

    if (viewport)

        viewport->setGridVisible(enabled);

    if (hintLabel)

        hintLabel->setText(enabled ? tr("Grid enabled") : tr("Grid hidden"));

}

void MainWindow::showViewSettingsDialog()

{

    if (!viewport)

        return;

    ViewSettingsDialog dialog(this);

    dialog.setFieldOfView(viewport->fieldOfView());

    dialog.setProjectionMode(viewport->projectionMode());

    if (dialog.exec() != QDialog::Accepted)

        return;

    viewport->setFieldOfView(dialog.fieldOfView());

    setProjectionMode(dialog.projectionMode(), false);

    const QString projectionText = dialog.projectionMode() == CameraController::ProjectionMode::Parallel ? tr("Parallel")

                                                                                                          : tr("Perspective");

    statusBar()->showMessage(tr("Field of view set to %1° (%2)")

                                 .arg(dialog.fieldOfView(), 0, 'f', 1)

                                 .arg(projectionText),

                             2000);

    syncViewSettingsUI();

    persistViewSettings();

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

void MainWindow::updateSelectionStatus()
{
    if (!selectionLabel || !viewport)
        return;
    Scene::Document* doc = viewport->getDocument();
    if (!doc) {
        selectionLabel->setText(tr("Selection: none"));
        return;
    }
    const auto& objects = doc->geometry().getObjects();
    int selectedCount = 0;
    int totalCount = 0;
    for (const auto& object : objects) {
        if (!object)
            continue;
        ++totalCount;
        if (object->isSelected())
            ++selectedCount;
    }
    QString textValue;
    if (totalCount == 0) {
        textValue = tr("Selection: none");
    } else if (selectedCount == 0) {
        textValue = tr("Selection: none (%1 total)").arg(totalCount);
    } else if (selectedCount == 1) {
        textValue = tr("Selection: 1 of %1").arg(totalCount);
    } else {
        textValue = tr("Selection: %1 of %2").arg(selectedCount).arg(totalCount);
    }
    if (selectionLabel->text() != textValue)
        selectionLabel->setText(textValue);
}

void MainWindow::updateFrameStats(double fps, double frameMs, int drawCalls)

{

    if (frameLabel)

        frameLabel->setText(tr("Frame: %1 ms • %2 draws • %3 fps").arg(frameMs, 0, 'f', 2).arg(drawCalls).arg(fps, 0, 'f', 1));

    updateSelectionStatus();
}

void MainWindow::handleMeasurementCommit(const QString& value, const QString& unitSystem)

{

    if (!toolManager)

        return;

    Tool::MeasurementKind kind = toolManager->getMeasurementKind();

    if (kind == Tool::MeasurementKind::None) {

        statusBar()->showMessage(tr("Active tool does not accept measurement overrides"), 2000);

        return;

    }

    MeasurementParseResult parsed = parseMeasurementValue(value, unitSystem, kind);

    if (!parsed.ok) {

        statusBar()->showMessage(tr("Invalid measurement: %1").arg(parsed.error), 3000);

        return;

    }

    if (!toolManager->applyMeasurementOverride(parsed.value)) {

        statusBar()->showMessage(tr("Measurement override is not applicable right now"), 3000);

        return;

    }

    if (measurementWidget)

        measurementWidget->clear();

    QString display = parsed.display.isEmpty() ? value.trimmed() : parsed.display;

    if (display.isEmpty()) {

        switch (kind) {

        case Tool::MeasurementKind::Distance:

            display = tr("%1 m").arg(parsed.value, 0, 'f', 3);

            break;

        case Tool::MeasurementKind::Angle:

            display = tr("%1 rad").arg(parsed.value, 0, 'f', 3);

            break;

        case Tool::MeasurementKind::Scale:

            display = tr("%1x").arg(parsed.value, 0, 'f', 2);

            break;

        default:

            break;

        }

    }

    statusBar()->showMessage(tr("Applied %1 override").arg(display), 2000);

}

