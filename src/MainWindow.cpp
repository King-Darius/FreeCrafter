#include "MainWindow.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "FileIO/Exporters/SceneExporter.h"
#include "FileIO/SceneIOFormat.h"
#include "GLViewport.h"
#include "NavigationConfig.h"
#include "NavigationPreferences.h"
#include "PalettePreferences.h"
#include "Scene/Document.h"
#include "Scene/SceneCommands.h"
#include "SunModel.h"
#include "Tools/ChamferTool.h"
#include "Tools/LoftTool.h"
#include "Tools/ToolManager.h"
#include "app/AutosaveManager.h"
#include "ui/ExternalReferenceDialog.h"
#include "ui/GuideManagerDialog.h"
#include "ui/ImageImportDialog.h"
#include "ui/ChamferOptionsDialog.h"
#include "ui/LoftOptionsDialog.h"
#include "ui/EnvironmentPanel.h"
#include "ui/InsertShapeDialog.h"
#include "ui/InspectorPanel.h"
#include "ui/MeasurementWidget.h"
#include "ui/PluginManagerDialog.h"
#include "ui/TerminalDock.h"
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
#include <QDir>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabBar>
#include <QUndoStack>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QStringList>
#include <QColor>
#include <QStackedWidget>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <initializer_list>
#include <Qt>

#include <algorithm>
#include <cmath>
#include <vector>

constexpr double kPi = 3.14159265358979323846;

namespace {

constexpr int kToolbarHeight = 48;

constexpr int kStatusHeight = 28;

const char* kSettingsGroup = "MainWindow";

QString primitiveDisplayName(Scene::PrimitiveType type)
{
    switch (type) {
    case Scene::PrimitiveType::Box:
        return QObject::tr("box");
    case Scene::PrimitiveType::Plane:
        return QObject::tr("plane");
    case Scene::PrimitiveType::Cylinder:
        return QObject::tr("cylinder");
    case Scene::PrimitiveType::Sphere:
        return QObject::tr("sphere");
    }
    return QObject::tr("primitive");
}

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

MeasurementParseResult parseCountMeasurement(const QString& raw)
{
    MeasurementParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty count");
        return result;
    }

    QString lower = sanitized.toLower();
    if (lower.endsWith(QStringLiteral("sides"))) {
        sanitized.chop(5);
    } else if (lower.endsWith(QStringLiteral("side"))) {
        sanitized.chop(4);
    } else if (sanitized.endsWith(QLatin1Char('s')) || sanitized.endsWith(QLatin1Char('S'))) {
        sanitized.chop(1);
    }

    if (sanitized.isEmpty()) {
        result.error = QObject::tr("invalid count");
        return result;
    }

    bool ok = false;
    int count = sanitized.toInt(&ok);
    if (!ok) {
        result.error = QObject::tr("invalid count");
        return result;
    }

    if (count < 3) {
        result.error = QObject::tr("sides must be at least 3");
        return result;
    }

    result.ok = true;
    result.value = static_cast<double>(count);
    result.display = QObject::tr("%1 sides").arg(count);
    return result;
}

MeasurementParseResult parseMeasurementValue(const QString& raw, const QString& unitSystem, Tool::MeasurementKind kind)

{

    switch (kind) {

    case Tool::MeasurementKind::Distance:

        return parseDistanceMeasurement(raw, unitSystem);

    case Tool::MeasurementKind::Angle:

        return parseAngleMeasurement(raw);

    case Tool::MeasurementKind::Count:

        return parseCountMeasurement(raw);

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

    case Tool::MeasurementKind::Count:

        return QObject::tr("Sides (e.g. 6 or 12s)");

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
    viewportWidget_ = viewport;

    centralLayout->addWidget(viewport, 1);

    setCentralWidget(central);

    navigationPrefs = std::make_unique<NavigationPreferences>(this);

    undoStack = new QUndoStack(this);
    commandStack = std::make_unique<Core::CommandStack>(undoStack);
    toolManager = std::make_unique<ToolManager>(viewport->getDocument(), viewport->getCamera(), commandStack.get());

    chamferDefaults_.radius = 0.1f;
    chamferDefaults_.segments = 6;
    chamferDefaults_.style = Phase6::CornerStyle::Chamfer;
    chamferDefaults_.tagHardEdges = true;

    loftDefaults_.sections = 8;
    loftDefaults_.closeRails = false;
    loftDefaults_.smoothNormals = true;
    loftDefaults_.twistDegrees = 0.0f;
    loftDefaults_.smoothingPasses = 1;
    loftDefaults_.symmetricPairing = false;

    toolManager->setGeometryChangedCallback([this]() {
        updateSelectionStatus();
        if (viewport)
            viewport->update();
        if (rightTray_)
            rightTray_->refreshPanels();
    });

    if (commandStack) {
        Core::CommandContext context;
        context.document = viewport->getDocument();
        context.geometry = viewport->getGeometry();
        context.geometryChanged = [this]() {
            if (toolManager)
                toolManager->notifyExternalGeometryChange();
            if (rightTray_)
                rightTray_->refreshPanels();
        };
        context.selectionChanged = [this](const std::vector<Scene::Document::ObjectId>&) {
            updateSelectionStatus();
            if (viewport)
                viewport->update();
            if (rightTray_)
                rightTray_->refreshPanels();
        };
        commandStack->setContext(context);
    }

    if (toolManager)
        toolManager->setCommandStack(commandStack.get());

    toolManager->setNavigationConfig(navigationPrefs->config());

    viewport->setToolManager(toolManager.get());

    viewport->setNavigationPreferences(navigationPrefs.get());
    initializeAutosave();

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
    bool storedGridVisible = true;

    {

        QSettings settings("FreeCrafter", "FreeCrafter");

        const QVariant storedTheme = settings.value(QStringLiteral("ui/darkMode"));
        if (storedTheme.isValid())
            darkTheme = storedTheme.toBool();
        else
            darkTheme = settings.value(QStringLiteral("%1/darkTheme").arg(kSettingsGroup), true).toBool();

        renderStyleChoice = renderStyleFromSetting(settings.value(QStringLiteral("%1/renderStyle").arg(kSettingsGroup), QStringLiteral("shadedEdges")).toString());

        showHiddenGeometry = settings.value(QStringLiteral("%1/showHiddenGeometry").arg(kSettingsGroup), false).toBool();

        storedViewPreset = settings.value(QStringLiteral("%1/viewPreset").arg(kSettingsGroup), storedViewPreset).toString();

        storedFov = settings.value(QStringLiteral("%1/viewFov").arg(kSettingsGroup), storedFov).toFloat();

        storedProjection = settings.value(QStringLiteral("%1/viewProjection").arg(kSettingsGroup), storedProjection).toString();
        storedGridVisible = settings.value(QStringLiteral("%1/showGrid").arg(kSettingsGroup), storedGridVisible).toBool();
        showFrameStatsHud = settings.value(QStringLiteral("%1/showFrameStatsHud").arg(kSettingsGroup), showFrameStatsHud).toBool();

        sunSettings.load(settings, QStringLiteral("Environment"));

    }

    viewport->setRenderStyle(renderStyleChoice);

    viewport->setShowHiddenGeometry(showHiddenGeometry);
    viewport->setShowGrid(storedGridVisible);
    viewport->setFrameStatsVisible(showFrameStatsHud);

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

    maybeRestoreAutosave();

}

MainWindow::~MainWindow()

{

    if (autosaveManager)
        autosaveManager->shutdown();

    persistWindowState();

}

void MainWindow::closeEvent(QCloseEvent* event)

{

    persistWindowState();

    if (autosaveManager)
        autosaveManager->shutdown();

    QMainWindow::closeEvent(event);

}

void MainWindow::initializeAutosave()

{

    autosaveManager = std::make_unique<AutosaveManager>(this);

    if (!viewport)

        return;

    autosaveManager->setDocument(viewport->getDocument());

    QSettings settings("FreeCrafter", "FreeCrafter");

    settings.beginGroup(QStringLiteral("Autosave"));

    const int interval = settings.value(QStringLiteral("intervalMinutes"), 5).toInt();

    const int retention = settings.value(QStringLiteral("retentionCount"), 5).toInt();

    currentDocumentPath = settings.value(QStringLiteral("lastSourcePath")).toString();

    settings.endGroup();

    autosaveManager->setIntervalMinutes(interval);

    autosaveManager->setRetentionCount(retention);

    autosaveManager->setSourcePath(currentDocumentPath);

    autosaveManager->setDocument(viewport->getDocument());

}

void MainWindow::maybeRestoreAutosave()

{

    if (!autosaveManager || !viewport)

        return;

    const auto latest = autosaveManager->latestAutosave();

    if (!latest)

        return;

    const QString timestamp = latest->timestamp.toLocalTime().toString(Qt::DefaultLocaleLongDate);

    QString prompt;

    if (currentDocumentPath.isEmpty()) {

        prompt = tr("An autosave from %1 was found. Do you want to restore it?").arg(timestamp);

    } else {

        prompt = tr("An autosave for %1 from %2 was found. Do you want to restore it?")

                     .arg(QFileInfo(currentDocumentPath).fileName(), timestamp);

    }

    const QMessageBox::StandardButton choice = QMessageBox::question(this,

                                                                      tr("Restore Autosave"),

                                                                      prompt,

                                                                      QMessageBox::Yes | QMessageBox::No,

                                                                      QMessageBox::Yes);

    if (choice != QMessageBox::Yes)

        return;

    Scene::Document* doc = viewport->getDocument();

    if (!doc)

        return;

    if (autosaveManager->restoreLatest(doc)) {

        viewport->update();

        updateSelectionStatus();

        if (statusBar())

            statusBar()->showMessage(tr("Autosave restored"), 3000);

    } else {

        QMessageBox::warning(this, tr("Restore Autosave"), tr("Failed to restore the autosave file."));

    }

}

void MainWindow::updateAutosaveSource(const QString& path, bool purgePreviousPrefix)

{

    QString previousPrefix;

    if (autosaveManager)

        previousPrefix = autosaveManager->currentPrefix();

    currentDocumentPath = path;

    if (autosaveManager) {

        if (purgePreviousPrefix && !previousPrefix.isEmpty())

            autosaveManager->clearAutosavesForPrefix(previousPrefix);

        autosaveManager->setSourcePath(currentDocumentPath);

    }

    persistAutosaveSettings();

}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    actionNew = fileMenu->addAction(tr("New"), this, &MainWindow::newFile);
    actionNew->setIcon(QIcon(QStringLiteral(":/icons/newfile.svg")));
    actionNew->setStatusTip(tr("Create a blank FreeCrafter document"));

    actionOpen = fileMenu->addAction(tr("Open..."), this, &MainWindow::openFile);
    actionOpen->setIcon(QIcon(QStringLiteral(":/icons/open.svg")));
    actionOpen->setStatusTip(tr("Open an existing FreeCrafter document"));

    actionImport = fileMenu->addAction(tr("Import..."), this, &MainWindow::importExternalModel);
    actionImport->setIcon(QIcon(QStringLiteral(":/icons/import.svg")));
    actionImport->setStatusTip(tr("Import external geometry into the active document"));

    actionSave = fileMenu->addAction(tr("Save"), this, &MainWindow::saveFile);
    actionSave->setIcon(QIcon(QStringLiteral(":/icons/save.svg")));
    actionSave->setStatusTip(tr("Save the active document"));

    actionSaveAs = fileMenu->addAction(tr("Save As..."), this, &MainWindow::saveFileAs);
    actionSaveAs->setStatusTip(tr("Save the active document to a new file"));

    QMenu* recentMenu = fileMenu->addMenu(tr("Recent"));
    QAction* recentPlaceholder = recentMenu->addAction(tr("No recent files yet"));
    recentPlaceholder->setEnabled(false);

    actionExport = fileMenu->addAction(tr("Export..."), this, &MainWindow::exportFile);
    actionExport->setIcon(QIcon(QStringLiteral(":/icons/export.svg")));
    actionExport->setStatusTip(tr("Export geometry to an interchange format"));

    fileMenu->addSeparator();

    actionExit = fileMenu->addAction(tr("Exit"), this, &QWidget::close);
    actionExit->setStatusTip(tr("Close FreeCrafter"));

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    actionUndo = editMenu->addAction(tr("Undo"), this, &MainWindow::onUndo);
    actionUndo->setIcon(QIcon(QStringLiteral(":/icons/undo.svg")));
    actionUndo->setShortcut(QKeySequence::Undo);
    actionUndo->setStatusTip(tr("Undo the previous action"));

    actionRedo = editMenu->addAction(tr("Redo"), this, &MainWindow::onRedo);
    actionRedo->setIcon(QIcon(QStringLiteral(":/icons/redo.svg")));
    actionRedo->setShortcut(QKeySequence::Redo);
    actionRedo->setStatusTip(tr("Redo the next action"));

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

    actionToggleProjection = viewMenu->addAction(tr("Parallel Projection"));
    actionToggleProjection->setCheckable(true);
    actionToggleProjection->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    actionToggleProjection->setStatusTip(tr("Toggle between perspective and parallel projections"));
    if (viewport) {
        QSignalBlocker blocker(actionToggleProjection);
        actionToggleProjection->setChecked(viewport->projectionMode() == CameraController::ProjectionMode::Parallel);
    }
    connect(actionToggleProjection, &QAction::toggled, this, [this](bool checked) {
        setProjectionMode(checked ? CameraController::ProjectionMode::Parallel : CameraController::ProjectionMode::Perspective);
    });

    actionViewSettings = viewMenu->addAction(tr("Camera Settings..."), this, &MainWindow::showViewSettingsDialog);
    actionViewSettings->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    actionViewSettings->setStatusTip(tr("Adjust field of view and projection settings"));

    QAction* actionSplitView = viewMenu->addAction(tr("Split View"), this, [this]() {
        statusBar()->showMessage(tr("Split view is coming soon"), 2000);
    });
    actionSplitView->setEnabled(false);

    actionToggleRightDock = viewMenu->addAction(tr("Toggle Right Panel"), this, &MainWindow::toggleRightDock);
    actionToggleRightDock->setStatusTip(tr("Show or hide the panels dock"));

    actionToggleTheme = viewMenu->addAction(tr("Toggle Dark Mode"));
    actionToggleTheme->setCheckable(true);
    actionToggleTheme->setStatusTip(tr("Switch between light and dark interface themes"));
    {
        QSignalBlocker blocker(actionToggleTheme);
        actionToggleTheme->setChecked(darkTheme);
    }
    connect(actionToggleTheme, &QAction::toggled, this, &MainWindow::setDarkTheme);
    updateThemeActionIcon();

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
    renderHiddenLineAction->setChecked(renderStyleChoice == Renderer::RenderStyle::HiddenLine);
    renderShadedAction->setChecked(renderStyleChoice == Renderer::RenderStyle::Shaded);
    renderShadedEdgesAction->setChecked(renderStyleChoice == Renderer::RenderStyle::ShadedWithEdges);
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

    gridAction = viewMenu->addAction(tr("Show Grid"));
    gridAction->setCheckable(true);
    gridAction->setIcon(QIcon(QStringLiteral(":/icons/grid.svg")));
    gridAction->setStatusTip(tr("Toggle the modeling grid visibility."));
    if (viewport) {
        gridAction->setChecked(viewport->showGrid());
    }
    connect(gridAction, &QAction::toggled, this, &MainWindow::toggleGrid);

    actionShowFrameStatsHud = viewMenu->addAction(tr("Show Frame Stats HUD"));
    actionShowFrameStatsHud->setCheckable(true);
    actionShowFrameStatsHud->setStatusTip(tr("Show rendering performance stats in the viewport."));
    actionShowFrameStatsHud->setChecked(showFrameStatsHud);
    connect(actionShowFrameStatsHud, &QAction::toggled, this, [this](bool checked) {
        showFrameStatsHud = checked;
        if (viewport)
            viewport->setFrameStatsVisible(checked);
        persistViewSettings();
        if (statusBar())
            statusBar()->showMessage(checked ? tr("Frame stats overlay shown") : tr("Frame stats overlay hidden"), 1500);
    });

    QMenu* insertMenu = menuBar()->addMenu(tr("&Insert"));
    QAction* insertShapesAction = insertMenu->addAction(tr("Shapes"), this, &MainWindow::showInsertShapesDialog);
    insertShapesAction->setStatusTip(tr("Open the primitive dialog to add a box, plane, cylinder, or sphere."));
    insertShapesAction->setToolTip(tr("Insert primitive geometry"));

    QAction* insertGuidesAction = insertMenu->addAction(tr("Guides"), this, &MainWindow::showGuideManager);
    insertGuidesAction->setStatusTip(tr("Open the guide manager to add or clear axis guides."));
    insertGuidesAction->setToolTip(tr("Manage drawing guides"));

    QAction* insertImagesAction = insertMenu->addAction(tr("Images"), this, &MainWindow::showImageImportDialog);
    insertImagesAction->setStatusTip(tr("Import an image as a reference plane in the scene."));
    insertImagesAction->setToolTip(tr("Insert reference image"));

    QAction* insertReferenceAction = insertMenu->addAction(tr("External Reference"), this, &MainWindow::showExternalReferenceDialog);
    insertReferenceAction->setStatusTip(tr("Link an external model as a proxy in the current document."));
    insertReferenceAction->setToolTip(tr("Link external model"));

    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    QAction* pluginManagerAction = toolsMenu->addAction(tr("Plugins..."), this, &MainWindow::showPluginManagerDialog);
    pluginManagerAction->setStatusTip(tr("Inspect discovered plugins and refresh the plugin search paths."));
    pluginManagerAction->setToolTip(tr("Open plugin manager"));
    actionPalette = toolsMenu->addAction(tr("Command Palette..."), this, &MainWindow::showCommandPalette);
    actionPalette->setIcon(QIcon(QStringLiteral(":/icons/search.svg")));

    advancedToolsMenu = toolsMenu->addMenu(tr("Advanced Tools"));
    chamferOptionsAction = advancedToolsMenu->addAction(tr("Chamfer Options..."), this, &MainWindow::showChamferOptionsDialog);
    if (chamferOptionsAction) {
        chamferOptionsAction->setStatusTip(tr("Adjust chamfer defaults including radius and segments."));
        chamferOptionsAction->setToolTip(tr("Configure chamfer tool"));
    }
    loftOptionsAction = advancedToolsMenu->addAction(tr("Loft Options..."), this, &MainWindow::showLoftOptionsDialog);
    if (loftOptionsAction) {
        loftOptionsAction->setStatusTip(tr("Adjust loft sections, twist, and smoothing."));
        loftOptionsAction->setToolTip(tr("Configure loft tool"));
    }

    QMenu* windowMenu = menuBar()->addMenu(tr("&Window"));
    QAction* newWindowAction = windowMenu->addAction(tr("New Window"), this, &MainWindow::spawnNewWindow);
    newWindowAction->setStatusTip(tr("Open a second FreeCrafter window."));
    newWindowAction->setToolTip(tr("Open another window"));

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    actionShortcuts = helpMenu->addAction(tr("Keyboard Shortcuts"), this, &MainWindow::showKeyboardShortcuts);
    actionShortcuts->setIcon(QIcon(QStringLiteral(":/icons/help.svg")));
    helpMenu->addAction(tr("About"), this, [this]() {
        QMessageBox::about(this, tr("About FreeCrafter"), tr("FreeCrafter - MIT Licensed CAD shell prototype."));
    });

    if (undoStack) {
        actionUndo->setEnabled(undoStack->canUndo());
        actionRedo->setEnabled(undoStack->canRedo());
        connect(undoStack, &QUndoStack::canUndoChanged, actionUndo, &QAction::setEnabled);
        connect(undoStack, &QUndoStack::canRedoChanged, actionRedo, &QAction::setEnabled);
        connect(undoStack, &QUndoStack::undoTextChanged, this, &MainWindow::updateUndoRedoActionText);
        connect(undoStack, &QUndoStack::redoTextChanged, this, &MainWindow::updateUndoRedoActionText);
        updateUndoRedoActionText();
    } else {
        actionUndo->setEnabled(false);
        actionRedo->setEnabled(false);
    }
}

void MainWindow::importExternalModel()
{
    QStringList wildcardFilters;
    QStringList formatFilters;
    const auto availableFormats = FileIO::availableSceneFormats();
    for (auto format : availableFormats) {
        const std::string extension = FileIO::formatExtension(format);
        if (!extension.empty()) {
            QString ext = QString::fromStdString(extension);
            if (ext.startsWith(QLatin1Char('.')))
                ext.remove(0, 1);
            if (!ext.isEmpty())
                wildcardFilters << QStringLiteral("*.%1").arg(ext);
        }
        const std::string filterString = FileIO::formatFilterString(format);
        if (!filterString.empty())
            formatFilters << QString::fromStdString(filterString);
    }

    QStringList filters;
    if (!wildcardFilters.isEmpty()) {
        filters << tr("Supported 3D Models (%1)").arg(wildcardFilters.join(QLatin1Char(' ')));
    }
    filters.append(formatFilters);
    if (filters.isEmpty()) {
        filters << tr("All Files (*)");
    }
    const QString filter = filters.join(QStringLiteral(";;"));

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Import Model"), QString(), filter);
    if (fileName.isEmpty())
        return;

    Scene::Document::FileFormat format = Scene::Document::FileFormat::Auto;
    const QFileInfo info(fileName);
    const QString suffix = info.suffix().toLower();
    if (!suffix.isEmpty()) {
        const auto sceneFormat = FileIO::sceneFormatFromExtension(suffix.toStdString());
        if (sceneFormat) {
            switch (*sceneFormat) {
            case FileIO::SceneFormat::OBJ:
                format = Scene::Document::FileFormat::Obj;
                break;
            case FileIO::SceneFormat::STL:
                format = Scene::Document::FileFormat::Stl;
                break;
            case FileIO::SceneFormat::GLTF:
                format = Scene::Document::FileFormat::Gltf;
                break;
            case FileIO::SceneFormat::FBX:
                format = Scene::Document::FileFormat::Fbx;
                break;
            default:
                break;
            }
        }

        if (format == Scene::Document::FileFormat::Auto) {
            if (suffix == QLatin1String("dxf")) {
                format = Scene::Document::FileFormat::Dxf;
            } else if (suffix == QLatin1String("dwg")) {
                format = Scene::Document::FileFormat::Dwg;
            }
        }
    }

    Scene::Document* doc = viewport ? viewport->getDocument() : nullptr;
    if (!doc) {
        statusBar()->showMessage(tr("No active document for import"), 2000);
        return;
    }

    const bool ok = doc->importExternalModel(fileName.toStdString(), format);
    if (ok) {
        statusBar()->showMessage(tr("Imported %1").arg(QFileInfo(fileName).fileName()), 1500);
        viewport->update();
    } else {
        QString reason = QString::fromStdString(doc->lastImportError());
        if (reason.isEmpty())
            reason = tr("Unknown import error");
        statusBar()->showMessage(tr("Failed to import %1: %2").arg(QFileInfo(fileName).fileName(), reason), 4000);
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
        statusBar()->showMessage(tr("No active document to export"), 4000);
        return;
    }

    const auto formats = FileIO::Exporters::supportedFormats();
    if (formats.empty()) {
        QMessageBox::warning(this, tr("No Export Formats"),
                             tr("No export formats are currently available. Enable plugins or features to export models."));
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
    if (!formats.empty()) {
        const auto& defaultFormat = formats.front();
        const std::string extension = FileIO::formatExtension(defaultFormat);
        if (!extension.empty()) {
            dialog.setDefaultSuffix(QString::fromStdString(extension.substr(1)));
        }
    }

    if (dialog.exec() == QDialog::Rejected)
        return;

    const QStringList selected = dialog.selectedFiles();
    if (selected.isEmpty())
        return;

    QString filePath = selected.first();
    FileIO::SceneFormat chosenFormat = formats.front();
    const QString selectedFilter = dialog.selectedNameFilter();
    for (auto format : formats) {
        if (selectedFilter == QString::fromStdString(FileIO::formatFilterString(format))) {
            chosenFormat = format;
            break;
        }
    }

    const QString extension = QString::fromStdString(FileIO::formatExtension(chosenFormat));
    if (!extension.isEmpty() && !filePath.endsWith(extension, Qt::CaseInsensitive)) {
        filePath += extension;
    }

    document->synchronizeWithGeometry();

    std::string errorMessage;
    if (!FileIO::Exporters::exportScene(*document, filePath.toStdString(), chosenFormat, &errorMessage)) {
        QString message = QString::fromStdString(errorMessage);
        if (message.isEmpty())
            message = tr("Unknown export error");
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Could not export the scene:\\n%1").arg(message));
        statusBar()->showMessage(tr("Export failed"), 5000);
        return;
    }

    statusBar()->showMessage(tr("Exported \"%1\"").arg(QFileInfo(filePath).fileName()), 4000);
}

void MainWindow::showInsertShapesDialog()
{
    if (!viewport) {
        statusBar()->showMessage(tr("No viewport available for inserting shapes."), 3000);
        return;
    }
    Scene::Document* document = viewport->getDocument();
    if (!document) {
        statusBar()->showMessage(tr("No active document for inserting shapes."), 3000);
        return;
    }
    if (!undoStack) {
        statusBar()->showMessage(tr("Undo stack unavailable; cannot insert shapes."), 3000);
        return;
    }

    InsertShapeDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    Scene::PrimitiveOptions options = dialog.selectedOptions();
    auto* command = new Scene::AddPrimitiveCommand(*document, options);
    command->redo();
    const std::string error = command->error();
    if (!error.empty()) {
        statusBar()->showMessage(tr("Failed to insert shape: %1").arg(QString::fromStdString(error)), 5000);
        delete command;
        return;
    }
    command->undo();
    undoStack->push(command);

    viewport->update();
    statusBar()->showMessage(tr("Inserted %1").arg(primitiveDisplayName(options.type)), 2500);
}

void MainWindow::showGuideManager()
{
    GeometryKernel::GuideState current;
    Scene::Document* document = nullptr;
    if (viewport)
        document = viewport->getDocument();
    if (document)
        current = document->geometry().getGuides();

    GuideManagerDialog dialog(current, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (!document || !undoStack) {
        statusBar()->showMessage(tr("Unable to update guides without an active document."), 3000);
        return;
    }

    GeometryKernel::GuideState next = dialog.selectedState();
    auto* command = new Scene::SetGuidesCommand(*document, next);
    undoStack->push(command);
    viewport->update();
    if (next.lines.empty() && next.points.empty() && next.angles.empty()) {
        statusBar()->showMessage(tr("Cleared all guides."), 2000);
    } else {
        statusBar()->showMessage(tr("Updated guides."), 2000);
    }
}

void MainWindow::showImageImportDialog()
{
    if (!viewport) {
        statusBar()->showMessage(tr("No viewport available for image import."), 3000);
        return;
    }
    Scene::Document* document = viewport->getDocument();
    if (!document) {
        statusBar()->showMessage(tr("No active document for image import."), 3000);
        return;
    }
    if (!undoStack) {
        statusBar()->showMessage(tr("Undo stack unavailable; cannot import image."), 3000);
        return;
    }

    ImageImportDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    Scene::ImagePlaneOptions options = dialog.options();
    if (options.filePath.empty()) {
        statusBar()->showMessage(tr("No image selected."), 2000);
        return;
    }

    auto* command = new Scene::AddImagePlaneCommand(*document, options);
    command->redo();
    const std::string error = command->error();
    if (!error.empty()) {
        statusBar()->showMessage(tr("Failed to insert image: %1").arg(QString::fromStdString(error)), 5000);
        delete command;
        return;
    }
    command->undo();
    undoStack->push(command);

    viewport->update();
    statusBar()->showMessage(tr("Inserted image plane \"%1\"").arg(QFileInfo(QString::fromStdString(options.filePath)).fileName()), 2500);
}

void MainWindow::showExternalReferenceDialog()
{
    if (!viewport) {
        statusBar()->showMessage(tr("No viewport available for linking references."), 3000);
        return;
    }
    Scene::Document* document = viewport->getDocument();
    if (!document) {
        statusBar()->showMessage(tr("No active document for linking references."), 3000);
        return;
    }
    if (!undoStack) {
        statusBar()->showMessage(tr("Undo stack unavailable; cannot link reference."), 3000);
        return;
    }

    ExternalReferenceDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    Scene::ExternalReferenceOptions options = dialog.options();
    if (options.filePath.empty()) {
        statusBar()->showMessage(tr("No reference selected."), 2000);
        return;
    }

    if (options.displayName.empty()) {
        options.displayName = QFileInfo(QString::fromStdString(options.filePath)).completeBaseName().toStdString();
    }

    auto* command = new Scene::LinkExternalReferenceCommand(*document, options);
    command->redo();
    const std::string error = command->error();
    if (!error.empty()) {
        statusBar()->showMessage(tr("Failed to link reference: %1").arg(QString::fromStdString(error)), 5000);
        delete command;
        return;
    }
    command->undo();
    undoStack->push(command);

    viewport->update();
    statusBar()->showMessage(tr("Linked external reference \"%1\"").arg(QString::fromStdString(options.displayName)), 2500);
}

void MainWindow::showPluginManagerDialog()
{
    PluginManagerDialog dialog(this);
    dialog.exec();
    statusBar()->showMessage(tr("Plugin manager closed."), 2000);
}

void MainWindow::spawnNewWindow()
{
    auto cleanup = [this]() {
        secondaryWindows.erase(std::remove_if(secondaryWindows.begin(), secondaryWindows.end(), [](const QPointer<MainWindow>& window) {
                                return window.isNull();
                            }),
                            secondaryWindows.end());
    };
    cleanup();

    MainWindow* window = new MainWindow();
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
    secondaryWindows.push_back(window);
    statusBar()->showMessage(tr("Opened a new window."), 2000);
}

void MainWindow::onUndo()
{
    if (!undoStack) {
        statusBar()->showMessage(tr("Undo stack unavailable"), 1500);
        return;
    }

    if (!undoStack->canUndo()) {
        statusBar()->showMessage(tr("Nothing to undo"), 1500);
        return;
    }

    const QString label = undoStack->undoText();
    undoStack->undo();
    updateUndoRedoActionText();
    statusBar()->showMessage(label.isEmpty() ? tr("Undid last action") : tr("Undid %1").arg(label), 1500);
}

void MainWindow::onRedo()
{
    if (!undoStack) {
        statusBar()->showMessage(tr("Undo stack unavailable"), 1500);
        return;
    }

    if (!undoStack->canRedo()) {
        statusBar()->showMessage(tr("Nothing to redo"), 1500);
        return;
    }

    const QString label = undoStack->redoText();
    undoStack->redo();
    updateUndoRedoActionText();
    statusBar()->showMessage(label.isEmpty() ? tr("Redid last action") : tr("Redid %1").arg(label), 1500);
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

    primaryToolbar->addAction(actionImport);

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
    actionTerminal->setCheckable(true);


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

    toolActionGroup = new QActionGroup(this);
    toolActionGroup->setExclusive(true);

    auto createToolAction = [this](QAction*& target,
                                   const QString& iconPath,
                                   const QString& label,
                                   void (MainWindow::*slot)(),
                                   const QString& statusTip) {
        target = new QAction(QIcon(iconPath), label, this);
        target->setCheckable(true);
        target->setStatusTip(statusTip);
        connect(target, &QAction::triggered, this, slot);
        toolActionGroup->addAction(target);
    };

    createToolAction(selectAction,
                     QStringLiteral(":/icons/select.png"),
                     tr("Select"),
                     &MainWindow::activateSelect,
                     tr("Select and edit existing geometry."));
    createToolAction(lineAction,
                     QStringLiteral(":/icons/line.png"),
                     tr("Line"),
                     &MainWindow::activateLine,
                     tr("Draw connected line segments."));
    createToolAction(rectangleAction,
                     QStringLiteral(":/icons/rectangle.png"),
                     tr("Rectangle"),
                     &MainWindow::activateRectangle,
                     tr("Create rectangles aligned to the axes."));
    createToolAction(rotatedRectangleAction,
                     QStringLiteral(":/icons/rotated_rectangle.svg"),
                     tr("Rotated Rectangle"),
                     &MainWindow::activateRotatedRectangle,
                     tr("Draw rectangles by edge, rotation, and height."));
    createToolAction(circleAction,
                     QStringLiteral(":/icons/circle.png"),
                     tr("Circle"),
                     &MainWindow::activateCircle,
                     tr("Draw circles by center and radius."));
    createToolAction(arcAction,
                     QStringLiteral(":/icons/arc.png"),
                     tr("3-Point Arc"),
                     &MainWindow::activateArc,
                     tr("Draw arcs using three points."));
    createToolAction(centerArcAction,
                     QStringLiteral(":/icons/center_arc.svg"),
                     tr("Center Arc"),
                     &MainWindow::activateCenterArc,
                     tr("Draw arcs from a center, start, and end angle."));
    createToolAction(tangentArcAction,
                     QStringLiteral(":/icons/tangent_arc.svg"),
                     tr("Tangent Arc"),
                     &MainWindow::activateTangentArc,
                     tr("Draw arcs tangent to a direction at the first point."));
    createToolAction(polygonAction,
                     QStringLiteral(":/icons/polygon.svg"),
                     tr("Polygon"),
                     &MainWindow::activatePolygon,
                     tr("Draw regular polygons by center and radius. Enter a side count to adjust."));
    createToolAction(bezierAction,
                     QStringLiteral(":/icons/bezier.svg"),
                     tr("Bezier"),
                     &MainWindow::activateBezier,
                     tr("Create cubic Bezier curves with adjustable handles."));
    createToolAction(freehandAction,
                     QStringLiteral(":/icons/freehand.svg"),
                     tr("Freehand"),
                     &MainWindow::activateFreehand,
                     tr("Sketch loose strokes that become polylines."));
    createToolAction(moveAction,
                     QStringLiteral(":/icons/move.png"),
                     tr("Move"),
                     &MainWindow::activateMove,
                     tr("Translate geometry to a new location."));
    createToolAction(rotateAction,
                     QStringLiteral(":/icons/rotate.png"),
                     tr("Rotate"),
                     &MainWindow::activateRotate,
                     tr("Rotate entities around a pivot."));
    createToolAction(scaleAction,
                     QStringLiteral(":/icons/scale.png"),
                     tr("Scale"),
                     &MainWindow::activateScale,
                     tr("Scale geometry about a point."));
    createToolAction(extrudeAction,
                     QStringLiteral(":/icons/pushpull.png"),
                     tr("Extrude"),
                     &MainWindow::activateExtrude,
                     tr("Push or pull faces to add volume."));
    createToolAction(chamferAction,
                     QStringLiteral(":/icons/offset.png"),
                     tr("Chamfer"),
                     &MainWindow::activateChamfer,
                     tr("Apply chamfers or fillets to a selected curve."));
    createToolAction(loftAction,
                     QStringLiteral(":/icons/followme.png"),
                     tr("Loft"),
                     &MainWindow::activateLoft,
                     tr("Create a lofted solid between two selected profiles."));
    createToolAction(sectionAction,
                     QStringLiteral(":/icons/section.png"),
                     tr("Section"),
                     &MainWindow::activateSection,
                     tr("Create section planes through the model."));
    createToolAction(panAction,
                     QStringLiteral(":/icons/pan.svg"),
                     tr("Pan"),
                     &MainWindow::activatePan,
                     tr("Pan the camera view."));
    createToolAction(orbitAction,
                     QStringLiteral(":/icons/orbit.svg"),
                     tr("Orbit"),
                     &MainWindow::activateOrbit,
                     tr("Orbit around the scene."));
    createToolAction(zoomAction,
                     QStringLiteral(":/icons/zoom.png"),
                     tr("Zoom"),
                     &MainWindow::activateZoom,
                     tr("Zoom the camera view."));
    createToolAction(measureAction,
                     QStringLiteral(":/icons/measure.svg"),
                     tr("Measure"),
                     &MainWindow::activateMeasure,
                     tr("Measure distances in the model."));

    toolOptionsToolbar = addToolBar(tr("Tool Options"));
    if (toolOptionsToolbar) {
        toolOptionsToolbar->setObjectName(QStringLiteral("ToolOptionsToolbar"));
        toolOptionsToolbar->setMovable(false);
        toolOptionsToolbar->setFloatable(false);
        toolOptionsToolbar->setIconSize(QSize(18, 18));

        auto* optionsContainer = new QWidget(toolOptionsToolbar);
        auto* optionsLayout = new QHBoxLayout(optionsContainer);
        optionsLayout->setContentsMargins(8, 4, 8, 4);
        optionsLayout->setSpacing(8);
        toolOptionsStack = new QStackedWidget(optionsContainer);
        optionsLayout->addWidget(toolOptionsStack);
        optionsContainer->setLayout(optionsLayout);
        auto* optionsAction = new QWidgetAction(toolOptionsToolbar);
        optionsAction->setDefaultWidget(optionsContainer);
        toolOptionsToolbar->addAction(optionsAction);

        toolOptionsPlaceholder = new QWidget(toolOptionsStack);
        if (toolOptionsPlaceholder) {
            auto* placeholderLayout = new QHBoxLayout(toolOptionsPlaceholder);
            placeholderLayout->setContentsMargins(0, 0, 0, 0);
            placeholderLayout->setSpacing(4);
            auto* placeholderLabel = new QLabel(tr("No active tool options"), toolOptionsPlaceholder);
            placeholderLayout->addWidget(placeholderLabel);
            placeholderLayout->addStretch(1);
        }
        if (toolOptionsStack && toolOptionsPlaceholder)
            toolOptionsStack->addWidget(toolOptionsPlaceholder);

        chamferOptionsWidget = new QWidget(toolOptionsStack);
        if (chamferOptionsWidget) {
            auto* chamferLayout = new QHBoxLayout(chamferOptionsWidget);
            chamferLayout->setContentsMargins(0, 0, 0, 0);
            chamferLayout->setSpacing(6);
            chamferRadiusSpin = new QDoubleSpinBox(chamferOptionsWidget);
            chamferRadiusSpin->setRange(0.001, 1000.0);
            chamferRadiusSpin->setDecimals(3);
            chamferRadiusSpin->setSingleStep(0.01);
            chamferRadiusSpin->setToolTip(tr("Chamfer radius"));
            chamferLayout->addWidget(chamferRadiusSpin);

            chamferSegmentsSpin = new QSpinBox(chamferOptionsWidget);
            chamferSegmentsSpin->setRange(1, 128);
            chamferSegmentsSpin->setToolTip(tr("Segment count"));
            chamferLayout->addWidget(chamferSegmentsSpin);

            chamferStyleCombo = new QComboBox(chamferOptionsWidget);
            chamferStyleCombo->addItem(tr("Fillet"), static_cast<int>(Phase6::CornerStyle::Fillet));
            chamferStyleCombo->addItem(tr("Chamfer"), static_cast<int>(Phase6::CornerStyle::Chamfer));
            chamferStyleCombo->setToolTip(tr("Corner style"));
            chamferLayout->addWidget(chamferStyleCombo);

            chamferHardEdgeCheck = new QCheckBox(tr("Hard"), chamferOptionsWidget);
            chamferHardEdgeCheck->setToolTip(tr("Tag resulting edges as hard"));
            chamferLayout->addWidget(chamferHardEdgeCheck);

            chamferDialogButton = new QToolButton(chamferOptionsWidget);
            chamferDialogButton->setIcon(QIcon(QStringLiteral(":/icons/settings.svg")));
            chamferDialogButton->setToolTip(tr("Open chamfer options"));
            chamferLayout->addWidget(chamferDialogButton);

            chamferApplyButton = new QToolButton(chamferOptionsWidget);
            chamferApplyButton->setText(tr("Apply"));
            chamferApplyButton->setToolTip(tr("Commit chamfer"));
            chamferApplyButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
            chamferLayout->addWidget(chamferApplyButton);

            chamferLayout->addStretch(1);
        }
        if (toolOptionsStack && chamferOptionsWidget)
            toolOptionsStack->addWidget(chamferOptionsWidget);

        loftOptionsWidget = new QWidget(toolOptionsStack);
        if (loftOptionsWidget) {
            auto* loftLayout = new QHBoxLayout(loftOptionsWidget);
            loftLayout->setContentsMargins(0, 0, 0, 0);
            loftLayout->setSpacing(6);

            loftSectionsSpin = new QSpinBox(loftOptionsWidget);
            loftSectionsSpin->setRange(2, 256);
            loftSectionsSpin->setToolTip(tr("Section count"));
            loftLayout->addWidget(loftSectionsSpin);

            loftTwistSpin = new QDoubleSpinBox(loftOptionsWidget);
            loftTwistSpin->setRange(-360.0, 360.0);
            loftTwistSpin->setDecimals(2);
            loftTwistSpin->setSingleStep(5.0);
            loftTwistSpin->setSuffix(QStringLiteral("°"));
            loftTwistSpin->setToolTip(tr("Twist"));
            loftLayout->addWidget(loftTwistSpin);

            loftSmoothingSpin = new QSpinBox(loftOptionsWidget);
            loftSmoothingSpin->setRange(0, 10);
            loftSmoothingSpin->setToolTip(tr("Smoothing passes"));
            loftLayout->addWidget(loftSmoothingSpin);

            loftCloseRailsCheck = new QCheckBox(tr("Close"), loftOptionsWidget);
            loftCloseRailsCheck->setToolTip(tr("Close rails"));
            loftLayout->addWidget(loftCloseRailsCheck);

            loftSmoothNormalsCheck = new QCheckBox(tr("Smooth"), loftOptionsWidget);
            loftSmoothNormalsCheck->setToolTip(tr("Smooth normals"));
            loftLayout->addWidget(loftSmoothNormalsCheck);

            loftSymmetryCheck = new QCheckBox(tr("Sym"), loftOptionsWidget);
            loftSymmetryCheck->setToolTip(tr("Symmetric pairing"));
            loftLayout->addWidget(loftSymmetryCheck);

            loftDialogButton = new QToolButton(loftOptionsWidget);
            loftDialogButton->setIcon(QIcon(QStringLiteral(":/icons/settings.svg")));
            loftDialogButton->setToolTip(tr("Open loft options"));
            loftLayout->addWidget(loftDialogButton);

            loftApplyButton = new QToolButton(loftOptionsWidget);
            loftApplyButton->setText(tr("Apply"));
            loftApplyButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
            loftApplyButton->setToolTip(tr("Commit loft"));
            loftLayout->addWidget(loftApplyButton);

            loftLayout->addStretch(1);
        }
        if (toolOptionsStack && loftOptionsWidget)
            toolOptionsStack->addWidget(loftOptionsWidget);
    }

    if (chamferRadiusSpin) {
        chamferRadiusSpin->setValue(chamferDefaults_.radius);
        connect(chamferRadiusSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            chamferDefaults_.radius = static_cast<float>(value);
            applyChamferDefaults();
        });
    }
    if (chamferSegmentsSpin) {
        chamferSegmentsSpin->setValue(chamferDefaults_.segments);
        connect(chamferSegmentsSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            chamferDefaults_.segments = value;
            applyChamferDefaults();
        });
    }
    if (chamferStyleCombo) {
        int styleIndex = chamferStyleCombo->findData(static_cast<int>(chamferDefaults_.style));
        chamferStyleCombo->setCurrentIndex(styleIndex < 0 ? 0 : styleIndex);
        connect(chamferStyleCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
            chamferDefaults_.style = static_cast<Phase6::CornerStyle>(chamferStyleCombo->itemData(index).toInt());
            applyChamferDefaults();
        });
    }
    if (chamferHardEdgeCheck) {
        chamferHardEdgeCheck->setChecked(chamferDefaults_.tagHardEdges);
        connect(chamferHardEdgeCheck, &QCheckBox::toggled, this, [this](bool checked) {
            chamferDefaults_.tagHardEdges = checked;
            applyChamferDefaults();
        });
    }
    if (chamferApplyButton) {
        connect(chamferApplyButton, &QToolButton::clicked, this, [this]() {
            applyChamferDefaults();
            if (toolManager)
                toolManager->commitActiveTool();
        });
    }
    if (chamferDialogButton)
        connect(chamferDialogButton, &QToolButton::clicked, this, &MainWindow::showChamferOptionsDialog);

    if (loftSectionsSpin) {
        loftSectionsSpin->setValue(loftDefaults_.sections);
        connect(loftSectionsSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            loftDefaults_.sections = value;
            applyLoftDefaults();
        });
    }
    if (loftTwistSpin) {
        loftTwistSpin->setValue(loftDefaults_.twistDegrees);
        connect(loftTwistSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            loftDefaults_.twistDegrees = static_cast<float>(value);
            applyLoftDefaults();
        });
    }
    if (loftSmoothingSpin) {
        loftSmoothingSpin->setValue(loftDefaults_.smoothingPasses);
        connect(loftSmoothingSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            loftDefaults_.smoothingPasses = value;
            applyLoftDefaults();
        });
    }
    if (loftCloseRailsCheck) {
        loftCloseRailsCheck->setChecked(loftDefaults_.closeRails);
        connect(loftCloseRailsCheck, &QCheckBox::toggled, this, [this](bool checked) {
            loftDefaults_.closeRails = checked;
            applyLoftDefaults();
        });
    }
    if (loftSmoothNormalsCheck) {
        loftSmoothNormalsCheck->setChecked(loftDefaults_.smoothNormals);
        connect(loftSmoothNormalsCheck, &QCheckBox::toggled, this, [this](bool checked) {
            loftDefaults_.smoothNormals = checked;
            applyLoftDefaults();
        });
    }
    if (loftSymmetryCheck) {
        loftSymmetryCheck->setChecked(loftDefaults_.symmetricPairing);
        connect(loftSymmetryCheck, &QCheckBox::toggled, this, [this](bool checked) {
            loftDefaults_.symmetricPairing = checked;
            applyLoftDefaults();
        });
    }
    if (loftApplyButton) {
        connect(loftApplyButton, &QToolButton::clicked, this, [this]() {
            applyLoftDefaults();
            if (toolManager)
                toolManager->commitActiveTool();
        });
    }
    if (loftDialogButton)
        connect(loftDialogButton, &QToolButton::clicked, this, &MainWindow::showLoftOptionsDialog);

    populateAdvancedToolsMenu();

    selectAction->setChecked(true);

    activateSelect();

    setRenderStyle(renderStyleChoice);

    refreshNavigationActionHints();

}

void MainWindow::createDockPanels()
{
    createLeftDock();
    createRightDock();
    createTerminalDock();
    customizeViewport();
}

void MainWindow::createLeftDock()
{
    if (leftDock_) {
        removeDockWidget(leftDock_);
        leftDock_->deleteLater();
    }

    auto collect = [](std::initializer_list<QAction*> list) {
        QList<QAction*> actions;
        for (QAction* action : list) {
            if (action)
                actions.append(action);
        }
        return actions;
    };

    QList<ToolGroup> groups;

    ToolGroup selectGroup;
    selectGroup.label = tr("SELECT");
    selectGroup.actions = collect({ selectAction });
    if (!selectGroup.actions.isEmpty())
        groups.append(selectGroup);

    ToolGroup drawGroup;
    drawGroup.label = tr("DRAW");
    drawGroup.actions = collect({ lineAction,
                                  rectangleAction,
                                  rotatedRectangleAction,
                                  circleAction,
                                  arcAction,
                                  centerArcAction,
                                  tangentArcAction,
                                  polygonAction,
                                  bezierAction,
                                  freehandAction });
    if (!drawGroup.actions.isEmpty())
        groups.append(drawGroup);

    ToolGroup modifyGroup;
    modifyGroup.label = tr("MODIFY");
    modifyGroup.actions = collect({ moveAction,
                                    rotateAction,
                                    scaleAction,
                                    extrudeAction,
                                    chamferAction,
                                    loftAction,
                                    sectionAction });
    if (!modifyGroup.actions.isEmpty())
        groups.append(modifyGroup);

    ToolGroup navGroup;
    navGroup.label = tr("NAV");
    navGroup.actions = collect({ panAction,
                                 orbitAction,
                                 zoomAction,
                                 measureAction,
                                 gridAction });
    if (!navGroup.actions.isEmpty())
        groups.append(navGroup);

    leftDock_ = new QDockWidget(tr("Tools"), this);
    leftDock_->setObjectName(QStringLiteral("ToolsDock"));
    leftDock_->setAllowedAreas(Qt::LeftDockWidgetArea);
    leftDock_->setFeatures(QDockWidget::NoDockWidgetFeatures);
    leftDock_->setMinimumWidth(LeftToolPalette::preferredWidth());
    leftDock_->setMaximumWidth(LeftToolPalette::preferredWidth());

    auto* palette = new LeftToolPalette(groups, leftDock_);
    leftDock_->setWidget(palette);
    addDockWidget(Qt::LeftDockWidgetArea, leftDock_);
}

void MainWindow::createRightDock()
{
    if (rightDock_) {
        removeDockWidget(rightDock_);
        rightDock_->deleteLater();
    }

    rightDock_ = new QDockWidget(tr("Panels"), this);
    rightDock_->setObjectName(QStringLiteral("PanelsDock"));
    rightDock_->setAllowedAreas(Qt::RightDockWidgetArea);
    rightDock_->setFeatures(QDockWidget::NoDockWidgetFeatures);
    rightDock_->setMinimumWidth(320);
    rightDock_->setMaximumWidth(420);

    Scene::Document* document = viewport ? viewport->getDocument() : nullptr;
    GeometryKernel* geometry = viewport ? viewport->getGeometry() : nullptr;
    Core::CommandStack* stack = commandStack.get();
    rightTray_ = new RightTray(document, geometry, stack, undoStack, rightDock_);
    rightDock_->setWidget(rightTray_);
    addDockWidget(Qt::RightDockWidgetArea, rightDock_);

    inspectorPanel = rightTray_ ? rightTray_->inspectorPanel() : nullptr;
    if (inspectorPanel) {
        connect(inspectorPanel, &InspectorPanel::shapeModified, this, [this]() {
            if (viewport)
                viewport->update();
            updateSelectionStatus();
        });
    }

    environmentPanel = rightTray_ ? rightTray_->environmentPanel() : nullptr;
    if (environmentPanel) {
        environmentPanel->setSettings(sunSettings);
        connect(environmentPanel, &EnvironmentPanel::settingsChanged, this, &MainWindow::handleSunSettingsChanged);
    }

    if (rightTray_)
        rightTray_->refreshPanels();
}

void MainWindow::createTerminalDock()
{
    if (terminalDock_) {
        removeDockWidget(terminalDock_);
        terminalDock_->deleteLater();
        terminalDock_ = nullptr;
        terminalWidget_ = nullptr;
    }

    initializingTerminalDock_ = true;

    terminalDock_ = new QDockWidget(tr("Terminal"), this);
    terminalDock_->setObjectName(QStringLiteral("TerminalDock"));
    terminalDock_->setAllowedAreas(Qt::BottomDockWidgetArea);
    terminalDock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    terminalDock_->setMinimumHeight(200);

    terminalWidget_ = new TerminalDock(terminalDock_);
    terminalDock_->setWidget(terminalWidget_);
    addDockWidget(Qt::BottomDockWidgetArea, terminalDock_);

    connect(terminalDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (actionTerminal) {
            QSignalBlocker blocker(actionTerminal);
            actionTerminal->setChecked(visible);
        }

        if (visible && terminalWidget_)
            terminalWidget_->focusCommandInput();

        if (!initializingTerminalDock_ && !isRestoringWindowState_)
            persistWindowState();
    });

    terminalDock_->hide();

    initializingTerminalDock_ = false;
}

void MainWindow::customizeViewport()
{
    if (!viewport)
        return;

    viewportWidget_ = viewport;
    viewport->setBackgroundPalette(QColor(179, 210, 240),
                                   QColor(188, 206, 188),
                                   QColor(140, 158, 140));

    if (!overlay_) {
        overlay_ = new ViewportOverlay(viewportWidget_);
        overlay_->setParent(viewportWidget_);
    }

    overlay_->raise();
    overlay_->move(viewportWidget_->width() - overlay_->width() - 10, 10);
    overlay_->show();

    connect(viewport, &GLViewport::viewportResized, this, &MainWindow::handleViewportResize, Qt::UniqueConnection);
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

    hotkeys.registerAction(QStringLiteral("file.import"), actionImport);
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

    hotkeys.registerAction(QStringLiteral("view.terminal"), actionTerminal);

    hotkeys.registerAction(QStringLiteral("theme.toggle"), actionToggleTheme);

    hotkeys.registerAction(QStringLiteral("tools.select"), selectAction);

    hotkeys.registerAction(QStringLiteral("tools.line"), lineAction);
    hotkeys.registerAction(QStringLiteral("tools.rectangle"), rectangleAction);
    hotkeys.registerAction(QStringLiteral("tools.rotatedRectangle"), rotatedRectangleAction);
    hotkeys.registerAction(QStringLiteral("tools.arc"), arcAction);
    hotkeys.registerAction(QStringLiteral("tools.centerArc"), centerArcAction);
    hotkeys.registerAction(QStringLiteral("tools.tangentArc"), tangentArcAction);
    hotkeys.registerAction(QStringLiteral("tools.circle"), circleAction);
    hotkeys.registerAction(QStringLiteral("tools.polygon"), polygonAction);
    hotkeys.registerAction(QStringLiteral("tools.bezier"), bezierAction);
    hotkeys.registerAction(QStringLiteral("tools.freehand"), freehandAction);

    hotkeys.registerAction(QStringLiteral("tools.move"), moveAction);

    hotkeys.registerAction(QStringLiteral("tools.rotate"), rotateAction);

    hotkeys.registerAction(QStringLiteral("tools.scale"), scaleAction);

    hotkeys.registerAction(QStringLiteral("tools.extrude"), extrudeAction);

    hotkeys.registerAction(QStringLiteral("tools.chamfer"), chamferAction);

    hotkeys.registerAction(QStringLiteral("tools.loft"), loftAction);

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

        if (actionImport) actionImport->setToolTip(format(actionImport, tr("Import")));
        if (actionSave) actionSave->setToolTip(format(actionSave, tr("Save")));
        if (actionExport) actionExport->setToolTip(format(actionExport, tr("Export")));

        if (actionUndo) actionUndo->setToolTip(format(actionUndo, tr("Undo")));

        if (actionRedo) actionRedo->setToolTip(format(actionRedo, tr("Redo")));

        if (actionPalette) actionPalette->setToolTip(format(actionPalette, tr("Command Palette")));

        if (actionTerminal) actionTerminal->setToolTip(format(actionTerminal, tr("Terminal")));

        if (actionToggleTheme) actionToggleTheme->setToolTip(format(actionToggleTheme, tr("Toggle Dark Mode")));

        if (selectAction) selectAction->setToolTip(format(selectAction, tr("Select")));

        if (lineAction) lineAction->setToolTip(format(lineAction, tr("Line")));
        if (rectangleAction) rectangleAction->setToolTip(format(rectangleAction, tr("Rectangle")));
        if (rotatedRectangleAction) rotatedRectangleAction->setToolTip(format(rotatedRectangleAction, tr("Rotated Rectangle")));
        if (arcAction) arcAction->setToolTip(format(arcAction, tr("3-Point Arc")));
        if (centerArcAction) centerArcAction->setToolTip(format(centerArcAction, tr("Center Arc")));
        if (tangentArcAction) tangentArcAction->setToolTip(format(tangentArcAction, tr("Tangent Arc")));
        if (circleAction) circleAction->setToolTip(format(circleAction, tr("Circle")));
        if (polygonAction) polygonAction->setToolTip(format(polygonAction, tr("Polygon")));
        if (bezierAction) bezierAction->setToolTip(format(bezierAction, tr("Bezier")));
        if (freehandAction) freehandAction->setToolTip(format(freehandAction, tr("Freehand")));

        if (moveAction) moveAction->setToolTip(format(moveAction, tr("Move")));

        if (rotateAction) rotateAction->setToolTip(format(rotateAction, tr("Rotate")));

        if (scaleAction) scaleAction->setToolTip(format(scaleAction, tr("Scale")));

        if (panAction) panAction->setToolTip(format(panAction, tr("Pan")));

        if (orbitAction) orbitAction->setToolTip(format(orbitAction, tr("Orbit")));

        if (zoomAction) zoomAction->setToolTip(format(zoomAction, tr("Zoom")));

        if (extrudeAction) extrudeAction->setToolTip(format(extrudeAction, tr("Extrude")));

        if (measureAction) measureAction->setToolTip(format(measureAction, tr("Measure")));

        if (gridAction) gridAction->setToolTip(format(gridAction, tr("Show Grid")));

    };

    updateTooltips();

    connect(&hotkeys, &HotkeyManager::shortcutsChanged, this, updateTooltips);

}

void MainWindow::applyThemeStylesheet()

{

    const QString path = darkTheme ? QStringLiteral(":/styles/dark.qss") : QStringLiteral(":/styles/light.qss");

    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {

        qApp->setStyleSheet(QString::fromUtf8(file.readAll()));

    }

    updateThemeActionIcon();

}

void MainWindow::restoreWindowState()

{

    QSettings settings("FreeCrafter", "FreeCrafter");

    const QVariant storedTheme = settings.value(QStringLiteral("ui/darkMode"));
    if (storedTheme.isValid())
        darkTheme = storedTheme.toBool();

    settings.beginGroup(kSettingsGroup);

    const QByteArray geometry = settings.value("geometry").toByteArray();

    const QByteArray state = settings.value("state").toByteArray();

    isRestoringWindowState_ = true;

    if (!geometry.isEmpty())

        restoreGeometry(geometry);

    if (!state.isEmpty())

        restoreState(state);

    isRestoringWindowState_ = false;

    if (!storedTheme.isValid())
        darkTheme = settings.value("darkTheme", darkTheme).toBool();

    settings.endGroup();

}

void MainWindow::persistWindowState()

{

    QSettings settings("FreeCrafter", "FreeCrafter");

    settings.setValue(QStringLiteral("ui/darkMode"), darkTheme);
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

    updateToolOptionsPanel(toolId);

}

void MainWindow::updateThemeActionIcon()

{

    if (!actionToggleTheme)

        return;

    const QString iconPath = darkTheme ? QStringLiteral(":/icons/theme_light.svg") : QStringLiteral(":/icons/theme_dark.svg");

    actionToggleTheme->setIcon(QIcon(iconPath));

}

void MainWindow::updateToolOptionsPanel(const QString& toolId)

{

    if (!toolOptionsStack)

        return;

    QWidget* target = toolOptionsPlaceholder ? toolOptionsPlaceholder : nullptr;

    if (toolId == QLatin1String("ChamferTool") && chamferOptionsWidget)

        target = chamferOptionsWidget;

    else if (toolId == QLatin1String("LoftTool") && loftOptionsWidget)

        target = loftOptionsWidget;

    if (target)

        toolOptionsStack->setCurrentWidget(target);

    syncActiveToolOptions();

}

void MainWindow::syncActiveToolOptions()

{

    if (!toolManager)

        return;

    Tool* active = toolManager->getActiveTool();

    if (auto* chamfer = dynamic_cast<ChamferTool*>(active)) {

        chamferDefaults_ = chamfer->roundCornerOptions();

        updateChamferControls(chamferDefaults_);

    } else if (auto* loft = dynamic_cast<LoftTool*>(active)) {

        loftDefaults_ = loft->loftOptions();

        updateLoftControls(loftDefaults_);

    }

}

void MainWindow::updateChamferControls(const Phase6::RoundCornerOptions& options)

{

    if (chamferRadiusSpin) {

        QSignalBlocker blocker(chamferRadiusSpin);

        chamferRadiusSpin->setValue(options.radius);

    }

    if (chamferSegmentsSpin) {

        QSignalBlocker blocker(chamferSegmentsSpin);

        chamferSegmentsSpin->setValue(std::max(1, options.segments));

    }

    if (chamferStyleCombo) {

        QSignalBlocker blocker(chamferStyleCombo);

        int index = chamferStyleCombo->findData(static_cast<int>(options.style));

        chamferStyleCombo->setCurrentIndex(index < 0 ? 0 : index);

    }

    if (chamferHardEdgeCheck) {

        QSignalBlocker blocker(chamferHardEdgeCheck);

        chamferHardEdgeCheck->setChecked(options.tagHardEdges);

    }

}

void MainWindow::updateLoftControls(const Phase6::LoftOptions& options)

{

    if (loftSectionsSpin) {

        QSignalBlocker blocker(loftSectionsSpin);

        loftSectionsSpin->setValue(std::max(2, options.sections));

    }

    if (loftTwistSpin) {

        QSignalBlocker blocker(loftTwistSpin);

        loftTwistSpin->setValue(options.twistDegrees);

    }

    if (loftSmoothingSpin) {

        QSignalBlocker blocker(loftSmoothingSpin);

        loftSmoothingSpin->setValue(std::max(0, options.smoothingPasses));

    }

    if (loftCloseRailsCheck) {

        QSignalBlocker blocker(loftCloseRailsCheck);

        loftCloseRailsCheck->setChecked(options.closeRails);

    }

    if (loftSmoothNormalsCheck) {

        QSignalBlocker blocker(loftSmoothNormalsCheck);

        loftSmoothNormalsCheck->setChecked(options.smoothNormals);

    }

    if (loftSymmetryCheck) {

        QSignalBlocker blocker(loftSymmetryCheck);

        loftSymmetryCheck->setChecked(options.symmetricPairing);

    }

}

void MainWindow::applyChamferDefaults()

{

    chamferDefaults_.radius = std::max(chamferDefaults_.radius, 0.001f);

    chamferDefaults_.segments = std::max(1, chamferDefaults_.segments);

    if (toolManager) {

        if (auto* chamfer = dynamic_cast<ChamferTool*>(toolManager->getActiveTool()))

            chamfer->setRoundCornerOptions(chamferDefaults_);

    }

    updateChamferControls(chamferDefaults_);

}

void MainWindow::applyLoftDefaults()

{

    loftDefaults_.sections = std::max(2, loftDefaults_.sections);

    loftDefaults_.smoothingPasses = std::max(0, loftDefaults_.smoothingPasses);

    if (toolManager) {

        if (auto* loft = dynamic_cast<LoftTool*>(toolManager->getActiveTool()))

            loft->setLoftOptions(loftDefaults_);

    }

    updateLoftControls(loftDefaults_);

}

void MainWindow::populateAdvancedToolsMenu()

{

    if (!advancedToolsMenu)

        return;

    advancedToolsMenu->clear();

    if (chamferAction)

        advancedToolsMenu->addAction(chamferAction);

    if (loftAction)

        advancedToolsMenu->addAction(loftAction);

    if ((chamferOptionsAction || loftOptionsAction) && (!advancedToolsMenu->isEmpty()))

        advancedToolsMenu->addSeparator();

    if (chamferOptionsAction)

        advancedToolsMenu->addAction(chamferOptionsAction);

    if (loftOptionsAction)

        advancedToolsMenu->addAction(loftOptionsAction);

}

void MainWindow::handleViewportResize(const QSize& size)
{
    if (!overlay_)
        return;

    overlay_->move(size.width() - overlay_->width() - 10, 10);
}

void MainWindow::updateUndoRedoActionText()
{
    if (!undoStack)
        return;

    if (actionUndo) {
        const QString undoLabel = undoStack->undoText();
        actionUndo->setText(undoLabel.isEmpty() ? tr("Undo") : tr("Undo %1").arg(undoLabel));
        actionUndo->setStatusTip(undoLabel.isEmpty() ? tr("Undo the previous action") : tr("Undo %1").arg(undoLabel));
    }

    if (actionRedo) {
        const QString redoLabel = undoStack->redoText();
        actionRedo->setText(redoLabel.isEmpty() ? tr("Redo") : tr("Redo %1").arg(redoLabel));
        actionRedo->setStatusTip(redoLabel.isEmpty() ? tr("Redo the next action") : tr("Redo %1").arg(redoLabel));
    }
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

    settings.setValue(QStringLiteral("showGrid"), viewport->showGrid());

    settings.setValue(QStringLiteral("showFrameStatsHud"), showFrameStatsHud);

    settings.endGroup();
    persistAutosaveSettings();

}

void MainWindow::persistAutosaveSettings() const

{

    QSettings settings("FreeCrafter", "FreeCrafter");

    settings.beginGroup(QStringLiteral("Autosave"));

    if (autosaveManager) {

        settings.setValue(QStringLiteral("intervalMinutes"), autosaveManager->intervalMinutes());

        settings.setValue(QStringLiteral("retentionCount"), autosaveManager->retentionCount());

    }

    settings.setValue(QStringLiteral("lastSourcePath"), currentDocumentPath);

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

    if (gridAction && viewport) {

        QSignalBlocker blocker(gridAction);

        gridAction->setChecked(viewport->showGrid());

    }

    if (actionShowFrameStatsHud && viewport) {

        QSignalBlocker blocker(actionShowFrameStatsHud);

        actionShowFrameStatsHud->setChecked(viewport->frameStatsVisible());

    }

    if (viewport)
        showFrameStatsHud = viewport->frameStatsVisible();

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

    constexpr float kAngleEpsilon = 0.1f;
    auto nearlyEqual = [](float a, float b, float epsilon) {
        return std::fabs(a - b) <= epsilon;
    };

    bool changed = previous.shadowsEnabled != current.shadowsEnabled

        || !nearlyEqual(previous.elevationDegrees, current.elevationDegrees, kAngleEpsilon)

        || !nearlyEqual(previous.azimuthDegrees, current.azimuthDegrees, kAngleEpsilon);

    if (!changed)

        return;

    if (!current.shadowsEnabled) {

        statusBar()->showMessage(tr("Shadows disabled"), 2000);

        return;

    }

    SunModel::Result result = SunModel::computeSunDirection(current.elevationDegrees,

                                                            current.azimuthDegrees);

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

    updateAutosaveSource(QString(), true);

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

        updateAutosaveSource(fn, false);

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

        updateAutosaveSource(fn, true);

        if (autosaveManager)

            autosaveManager->clearAutosaves();

        statusBar()->showMessage(tr("Saved %1").arg(QFileInfo(fn).fileName()), 1500);

    } else {

        statusBar()->showMessage(tr("Failed to save %1").arg(QFileInfo(fn).fileName()), 2000);

    }

}

void MainWindow::saveFileAs()

{

    saveFile();

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

    auto* autosaveIntervalLabel = new QLabel(tr("Autosave every (minutes, 0 disables)"), &dialog);

    autosaveIntervalLabel->setWordWrap(true);

    layout->addWidget(autosaveIntervalLabel);

    auto* autosaveIntervalSpin = new QSpinBox(&dialog);

    autosaveIntervalSpin->setRange(0, 120);

    autosaveIntervalSpin->setSuffix(tr(" min"));

    autosaveIntervalSpin->setValue(autosaveManager ? autosaveManager->intervalMinutes() : 5);

    layout->addWidget(autosaveIntervalSpin);

    auto* autosaveRetentionLabel = new QLabel(tr("Keep autosave revisions"), &dialog);

    autosaveRetentionLabel->setWordWrap(true);

    layout->addWidget(autosaveRetentionLabel);

    auto* autosaveRetentionSpin = new QSpinBox(&dialog);

    autosaveRetentionSpin->setRange(1, 50);

    autosaveRetentionSpin->setValue(autosaveManager ? autosaveManager->retentionCount() : 5);

    layout->addWidget(autosaveRetentionSpin);

    if (autosaveManager) {

        const QString autosavePath = QDir::toNativeSeparators(autosaveManager->autosaveDirectory());

        if (!autosavePath.isEmpty()) {

            auto* autosavePathLabel = new QLabel(tr("Autosaves are stored in %1").arg(autosavePath), &dialog);

            autosavePathLabel->setWordWrap(true);

            layout->addWidget(autosavePathLabel);

        }

    }

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

    if (autosaveManager) {

        autosaveManager->setIntervalMinutes(autosaveIntervalSpin->value());

        autosaveManager->setRetentionCount(autosaveRetentionSpin->value());

    }

    persistAutosaveSettings();

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
    if (!rightDock_)
        return;

    rightDock_->setVisible(!rightDock_->isVisible());
}

void MainWindow::setDarkTheme(bool enabled)
{
    if (darkTheme == enabled)
        return;

    darkTheme = enabled;
    applyThemeStylesheet();

    if (actionToggleTheme && actionToggleTheme->isChecked() != enabled) {
        QSignalBlocker blocker(actionToggleTheme);
        actionToggleTheme->setChecked(enabled);
    }

    persistWindowState();
}

void MainWindow::runTask()

{

    taskLabel->setText(tr("Running task..."));

    QTimer::singleShot(1200, this, [this]() {

        taskLabel->setText(tr("No background tasks"));

    });

}

void MainWindow::toggleTerminalDock()

{

    if (!terminalDock_)
        return;

    const bool shouldShow = !terminalDock_->isVisible();
    terminalDock_->setVisible(shouldShow);

    if (shouldShow) {
        terminalDock_->raise();
        terminalDock_->activateWindow();
        if (terminalWidget_)
            terminalWidget_->focusCommandInput();
    }

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

void MainWindow::activateArc()
{
    setActiveTool(arcAction, QStringLiteral("Arc"),
        tr("Arc: Click the first endpoint, click the second endpoint, then click to define the bulge."));
}

void MainWindow::activateCenterArc()
{
    setActiveTool(centerArcAction, QStringLiteral("CenterArc"),
        tr("Center Arc: Click the center, click the start angle, then click to set the end angle."));
}

void MainWindow::activateTangentArc()
{
    setActiveTool(tangentArcAction, QStringLiteral("TangentArc"),
        tr("Tangent Arc: Click the start point, click to set tangent direction, then click the end point."));
}

void MainWindow::activateCircle()
{
    setActiveTool(circleAction, QStringLiteral("Circle"),
        tr("Circle: Click to set the center, then click again to define the radius."));
}

void MainWindow::activatePolygon()
{
    setActiveTool(polygonAction, QStringLiteral("Polygon"),
        tr("Polygon: Click to set the center, then click again to set the radius. Type a side count to change segments."));
}

void MainWindow::activateRotatedRectangle()
{
    setActiveTool(rotatedRectangleAction, QStringLiteral("RotatedRectangle"),
        tr("Rotated Rectangle: Click two corners for the base edge, then click to set height."));
}

void MainWindow::activateFreehand()
{
    setActiveTool(freehandAction, QStringLiteral("Freehand"),
        tr("Freehand: Click and drag to sketch a polyline stroke."));
}

void MainWindow::activateBezier()
{
    setActiveTool(bezierAction, QStringLiteral("Bezier"),
        tr("Bezier: Click start and end anchors, then place the two handles."));
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

void MainWindow::activateChamfer()

{

    setActiveTool(chamferAction,
                  QStringLiteral("ChamferTool"),
                  tr("Chamfer: Select a closed curve, adjust radius, then click Apply."));

    applyChamferDefaults();

}

void MainWindow::activateLoft()

{

    setActiveTool(loftAction,
                  QStringLiteral("LoftTool"),
                  tr("Loft: Select two profiles, adjust sections, then click Apply."));

    applyLoftDefaults();

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

    if (!viewport)

        return;

    viewport->setShowGrid(enabled);

    if (hintLabel)

        hintLabel->setText(enabled ? tr("Grid enabled") : tr("Grid hidden"));

    if (statusBar())

        statusBar()->showMessage(enabled ? tr("Grid enabled") : tr("Grid hidden"), 2000);

    persistViewSettings();

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

void MainWindow::showChamferOptionsDialog()

{

    ChamferOptionsDialog dialog(this);

    dialog.setOptions(chamferDefaults_);

    if (dialog.exec() == QDialog::Accepted) {

        chamferDefaults_ = dialog.options();

        updateChamferControls(chamferDefaults_);

        applyChamferDefaults();

    }

}

void MainWindow::showLoftOptionsDialog()

{

    LoftOptionsDialog dialog(this);

    dialog.setOptions(loftDefaults_);

    if (dialog.exec() == QDialog::Accepted) {

        loftDefaults_ = dialog.options();

        updateLoftControls(loftDefaults_);

        applyLoftDefaults();

    }

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
    if (!selectionLabel || !viewport) {
        if (inspectorPanel)
            inspectorPanel->updateSelection(nullptr, {});
        return;
    }
    Scene::Document* doc = viewport->getDocument();
    if (!doc) {
        selectionLabel->setText(tr("Selection: none"));
        if (inspectorPanel)
            inspectorPanel->updateSelection(nullptr, {});
        return;
    }
    const auto& objects = doc->geometry().getObjects();
    int selectedCount = 0;
    int totalCount = 0;
    std::vector<GeometryObject*> selectedObjects;
    selectedObjects.reserve(objects.size());
    for (const auto& object : objects) {
        if (!object)
            continue;
        ++totalCount;
        if (object->isSelected()) {
            ++selectedCount;
            selectedObjects.push_back(object.get());
        }
    }
    if (inspectorPanel)
        inspectorPanel->updateSelection(&doc->geometry(), selectedObjects);
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

    syncActiveToolOptions();

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

        case Tool::MeasurementKind::Count:

            display = tr("%1 sides").arg(static_cast<int>(std::lround(parsed.value)));

            break;

        case Tool::MeasurementKind::Scale:

            display = tr("%1x").arg(parsed.value, 0, 'f', 2);

            break;

        default:

            break;

        }

    }

    statusBar()->showMessage(tr("Applied %1 override").arg(display), 2000);

    if (viewport)
        viewport->update();

}



