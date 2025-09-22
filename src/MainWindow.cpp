#include "MainWindow.h"

#include <QAction>
#include <QComboBox>
#include <QIcon>
#include <QDockWidget>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QCloseEvent>
#include <QKeySequence>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QSize>

#include <cmath>

#include "GLViewport.h"
#include "Hotkeys/HotkeyEditorDialog.h"
#include "Hotkeys/HotkeyManager.h"

namespace
{
constexpr double kInchToMeter = 0.0254;
constexpr double kFootToMeter = 0.3048;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(tr("FreeCrafter"));
    resize(1280, 820);
    setDockOptions(QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks);

    viewport = new GLViewport(this);
    setCentralWidget(viewport);

    toolManager = std::make_unique<ToolManager>(viewport->getGeometry(), viewport->getCamera());
    viewport->setToolManager(toolManager.get());

    hotkeyManager = std::make_unique<HotkeyManager>(this);

    createMenus();
    createToolbars();
    createDockPanels();
    createStatusWidgets();
    loadDefaultHotkeys();
    loadSettings();
    updateToolUi();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    newFileAction = fileMenu->addAction(tr("New"), this, &MainWindow::newFile);
    openFileAction = fileMenu->addAction(tr("Open..."), this, &MainWindow::openFile);
    saveFileAction = fileMenu->addAction(tr("Save..."), this, &MainWindow::saveFile);
    fileMenu->addSeparator();
    importHotkeysAction = fileMenu->addAction(tr("Import Hotkey Map..."), this, &MainWindow::importHotkeyMap);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), this, &QWidget::close);

    if (hotkeyManager)
    {
        hotkeyManager->registerAction(QStringLiteral("file.new"), tr("New File"), newFileAction, QKeySequence::New);
        hotkeyManager->registerAction(QStringLiteral("file.open"), tr("Open File"), openFileAction, QKeySequence::Open);
        hotkeyManager->registerAction(QStringLiteral("file.save"), tr("Save File"), saveFileAction, QKeySequence::Save);
    }

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("Undo"));
    editMenu->addAction(tr("Redo"));

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    zoomExtentsAction = viewMenu->addAction(QIcon(":/icons/zoom_extents.png"), tr("Zoom Extents"));
    connect(zoomExtentsAction, &QAction::triggered, [this]() {
        viewport->getCamera()->setDistance(15.0f);
        viewport->update();
        statusBar()->showMessage(tr("Zoomed to scene extents"), 2000);
    });
    if (hotkeyManager)
    {
        hotkeyManager->registerAction(QStringLiteral("view.zoomExtents"), tr("Zoom Extents"), zoomExtentsAction, QKeySequence(QStringLiteral("Shift+Z")));
    }

    menuBar()->addMenu(tr("&Camera"));
    menuBar()->addMenu(tr("&Draw"));

    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    hotkeyEditorAction = toolsMenu->addAction(tr("Hotkey Editor..."), this, &MainWindow::showHotkeyEditor);

    menuBar()->addMenu(tr("&Window"));
    menuBar()->addMenu(tr("&Help"));
}

void MainWindow::createToolbars()
{
    auto* fileBar = new QToolBar(tr("File"), this);
    fileBar->setObjectName(QStringLiteral("FileToolbar"));
    fileBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    if (newFileAction) fileBar->addAction(newFileAction);
    if (openFileAction) fileBar->addAction(openFileAction);
    if (saveFileAction) fileBar->addAction(saveFileAction);
    addToolBar(Qt::TopToolBarArea, fileBar);

    addToolBarBreak(Qt::TopToolBarArea);

    auto* navigationBar = new QToolBar(tr("Navigation"), this);
    navigationBar->setObjectName(QStringLiteral("NavigationToolbar"));
    navigationBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    QAction* orbitAction = navigationBar->addAction(QIcon(":/icons/orbit.png"), tr("Orbit"));
    QAction* panAction = navigationBar->addAction(QIcon(":/icons/pan.png"), tr("Pan"));
    QAction* zoomAction = navigationBar->addAction(QIcon(":/icons/zoom.png"), tr("Zoom"));

    connect(orbitAction, &QAction::triggered, [this]() {
        statusBar()->showMessage(tr("Orbit the camera with the mouse (MMB)."), 2500);
    });
    connect(panAction, &QAction::triggered, [this]() {
        statusBar()->showMessage(tr("Pan the camera by dragging with Shift+MMB."), 2500);
    });
    connect(zoomAction, &QAction::triggered, [this]() {
        statusBar()->showMessage(tr("Use the scroll wheel to zoom."), 2500);
    });

    addToolBar(Qt::TopToolBarArea, navigationBar);

    auto* modelingBar = new QToolBar(tr("Modeling"), this);
    modelingBar->setObjectName(QStringLiteral("ModelingToolbar"));
    modelingBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    modelingBar->setIconSize(QSize(24, 24));

    selectAction = modelingBar->addAction(QIcon(":/icons/select.png"), tr("Select"), this, &MainWindow::activateSelect);
    sketchAction = modelingBar->addAction(QIcon(":/icons/line.png"), tr("Sketch"), this, &MainWindow::activateSketch);
    extrudeAction = modelingBar->addAction(QIcon(":/icons/pushpull.png"), tr("Extrude"), this, &MainWindow::activateExtrude);

    modelingBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
    addToolBar(Qt::LeftToolBarArea, modelingBar);

    if (hotkeyManager)
    {
        hotkeyManager->registerAction(QStringLiteral("tool.select"), tr("Select Tool"), selectAction, QKeySequence(Qt::Key_Space));
        hotkeyManager->registerAction(QStringLiteral("tool.sketch"), tr("Sketch Tool"), sketchAction, QKeySequence(Qt::Key_L));
        hotkeyManager->registerAction(QStringLiteral("tool.extrude"), tr("Extrude Tool"), extrudeAction, QKeySequence(Qt::Key_P));
    }
}

void MainWindow::createDockPanels()
{
    QDockWidget* tray = new QDockWidget(tr("Default Tray"), this);
    tray->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    tray->setWidget(new QWidget(tray));
    addDockWidget(Qt::RightDockWidgetArea, tray);

    auto* outliner = new QDockWidget(tr("Outliner"), this);
    outliner->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    auto* tree = new QTreeWidget(outliner);
    tree->setHeaderLabels(QStringList{tr("Scene Objects")});
    tree->addTopLevelItem(new QTreeWidgetItem(QStringList{tr("Active Model") }));
    outliner->setWidget(tree);
    addDockWidget(Qt::LeftDockWidgetArea, outliner);

    auto* inspector = new QDockWidget(tr("Inspector"), this);
    inspector->setAllowedAreas(Qt::RightDockWidgetArea);
    auto* list = new QListWidget(inspector);
    list->addItem(tr("Properties coming soon"));
    inspector->setWidget(list);
    tabifyDockWidget(tray, inspector);
}

void MainWindow::createStatusWidgets()
{
    hintLabel = new QLabel(tr("Ready"), this);
    measurementBox = new QLineEdit(this);
    measurementBox->setPlaceholderText(tr("Measurements"));
    measurementBox->setToolTip(tr("Type values and press Enter to override tool input."));
    measurementBox->setClearButtonEnabled(true);

    unitLabel = new QLabel(tr("Units:"), this);
    unitSelector = new QComboBox(this);
    unitSelector->addItem(tr("Meters"), QVariant::fromValue(static_cast<int>(UnitSystem::MetricMeters)));
    unitSelector->addItem(tr("Feet + Inches"), QVariant::fromValue(static_cast<int>(UnitSystem::ImperialFeetInches)));
    unitSelector->addItem(tr("Inches"), QVariant::fromValue(static_cast<int>(UnitSystem::ImperialInches)));

    statusBar()->addWidget(hintLabel, 1);
    statusBar()->addPermanentWidget(unitLabel);
    statusBar()->addPermanentWidget(unitSelector);
    statusBar()->addPermanentWidget(measurementBox, 0);

    connect(measurementBox, &QLineEdit::returnPressed, this, &MainWindow::applyMeasurementOverride);
    connect(unitSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::unitChanged);
}

void MainWindow::loadDefaultHotkeys()
{
    if (!hotkeyManager)
        return;
    hotkeyManager->loadFromResource(QStringLiteral(":/hotkeys/default.json"));
}

void MainWindow::loadSettings()
{
    QSettings settings(QStringLiteral("FreeCrafter"), QStringLiteral("FreeCrafter"));
    const QByteArray geometry = settings.value(QStringLiteral("MainWindow/geometry")).toByteArray();
    const QByteArray state = settings.value(QStringLiteral("MainWindow/state")).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    if (!state.isEmpty()) {
        restoreState(state);
    }
    int unitValue = settings.value(QStringLiteral("MainWindow/unit"), static_cast<int>(UnitSystem::MetricMeters)).toInt();
    currentUnits = static_cast<UnitSystem>(unitValue);
    if (unitSelector) {
        for (int i = 0; i < unitSelector->count(); ++i) {
            if (unitSelector->itemData(i).toInt() == unitValue) {
                unitSelector->setCurrentIndex(i);
                break;
            }
        }
    }
}

void MainWindow::saveSettings()
{
    QSettings settings(QStringLiteral("FreeCrafter"), QStringLiteral("FreeCrafter"));
    settings.setValue(QStringLiteral("MainWindow/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("MainWindow/state"), saveState());
    settings.setValue(QStringLiteral("MainWindow/unit"), static_cast<int>(currentUnits));
}

void MainWindow::newFile()
{
    viewport->getGeometry()->clear();
    viewport->update();
    statusBar()->showMessage(tr("Started a new model."), 2000);
}

void MainWindow::openFile()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open FreeCrafter Model"), QString(), tr("FreeCrafter (*.fcm)"));
    if (fn.isEmpty()) return;
    viewport->getGeometry()->loadFromFile(fn.toStdString());
    viewport->update();
    statusBar()->showMessage(tr("Loaded %1").arg(fn), 2000);
}

void MainWindow::saveFile()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save FreeCrafter Model"), QString(), tr("FreeCrafter (*.fcm)"));
    if (fn.isEmpty()) return;
    viewport->getGeometry()->saveToFile(fn.toStdString());
    statusBar()->showMessage(tr("Saved %1").arg(fn), 2000);
}

void MainWindow::activateSelect()
{
    toolManager->activateTool("SelectionTool");
    updateToolUi();
}

void MainWindow::activateSketch()
{
    toolManager->activateTool("SketchTool");
    updateToolUi();
}

void MainWindow::activateExtrude()
{
    toolManager->activateTool("ExtrudeTool");
    updateToolUi();
}

void MainWindow::importHotkeyMap()
{
    if (!hotkeyManager) return;
    QString path = QFileDialog::getOpenFileName(this, tr("Import Hotkey Map"), QString(), tr("JSON Files (*.json)"));
    if (path.isEmpty()) return;
    if (hotkeyManager->loadFromFile(path)) {
        statusBar()->showMessage(tr("Hotkeys imported from %1").arg(path), 3000);
    } else {
        QMessageBox::warning(this, tr("Import Failed"), tr("The selected file could not be read."));
    }
}

void MainWindow::showHotkeyEditor()
{
    if (!hotkeyManager) return;
    HotkeyEditorDialog dialog(hotkeyManager.get(), this);
    dialog.exec();
}

void MainWindow::applyMeasurementOverride()
{
    if (!toolManager) return;
    Tool* tool = toolManager->getActiveTool();
    if (!tool || !tool->acceptsNumericInput()) {
        statusBar()->showMessage(tr("The active tool does not accept typed measurements."), 3000);
        return;
    }

    double value = 0.0;
    if (!parseMeasurement(measurementBox->text(), value)) {
        statusBar()->showMessage(tr("Unable to parse measurement."), 4000);
        return;
    }

    if (tool->applyNumericInput(value)) {
        measurementBox->setText(formatMeasurement(value));
        statusBar()->showMessage(tr("Measurement applied."), 2000);
    } else {
        statusBar()->showMessage(tr("Active tool rejected measurement."), 4000);
    }
}

void MainWindow::unitChanged(int index)
{
    if (!unitSelector) return;
    int data = unitSelector->itemData(index).toInt();
    currentUnits = static_cast<UnitSystem>(data);
    updateToolUi();
}

void MainWindow::updateToolUi()
{
    if (!toolManager) return;
    Tool* tool = toolManager->getActiveTool();
    if (!tool) return;

    if (hintLabel) {
        QString hint = QString::fromStdString(tool->getHint());
        if (hint.isEmpty()) {
            hint = tr("Ready");
        }
        hintLabel->setText(hint);
    }
    if (measurementBox) {
        measurementBox->setEnabled(tool->acceptsNumericInput());
        QString prompt = QString::fromStdString(tool->getMeasurementPrompt());
        if (prompt.isEmpty()) {
            prompt = tr("Measurements");
        }
        QString unitHint;
        switch (currentUnits) {
        case UnitSystem::MetricMeters:
            unitHint = tr("m");
            break;
        case UnitSystem::ImperialFeetInches:
            unitHint = tr("ft/in");
            break;
        case UnitSystem::ImperialInches:
            unitHint = tr("in");
            break;
        }
        measurementBox->setPlaceholderText(prompt + QStringLiteral(" [%1]").arg(unitHint));
        if (!tool->acceptsNumericInput()) {
            measurementBox->clear();
        }
    }
}

bool MainWindow::parseMeasurement(const QString& text, double& value) const
{
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    bool ok = false;
    switch (currentUnits) {
    case UnitSystem::MetricMeters:
        value = trimmed.toDouble(&ok);
        return ok;
    case UnitSystem::ImperialInches:
        value = trimmed.toDouble(&ok);
        if (!ok) return false;
        value *= kInchToMeter;
        return true;
    case UnitSystem::ImperialFeetInches:
    {
        int footPos = trimmed.indexOf('\'');
        double feet = 0.0;
        double inches = 0.0;
        if (footPos >= 0) {
            feet = trimmed.left(footPos).trimmed().toDouble(&ok);
            if (!ok) return false;
            trimmed = trimmed.mid(footPos + 1);
            trimmed = trimmed.trimmed();
        } else {
            feet = trimmed.toDouble(&ok);
            if (!ok) return false;
            value = feet * kFootToMeter;
            return true;
        }

        int inchPos = trimmed.indexOf('"');
        if (inchPos >= 0) {
            QString inchPart = trimmed.left(inchPos).trimmed();
            inches = inchPart.isEmpty() ? 0.0 : inchPart.toDouble(&ok);
            if (!ok) return false;
        } else {
            inches = trimmed.trimmed().isEmpty() ? 0.0 : trimmed.trimmed().toDouble(&ok);
            if (!ok) return false;
        }
        value = feet * kFootToMeter + inches * kInchToMeter;
        return true;
    }
    }
    return false;
}

QString MainWindow::formatMeasurement(double value) const
{
    switch (currentUnits) {
    case UnitSystem::MetricMeters:
        return QString::number(value, 'f', 3) + tr(" m");
    case UnitSystem::ImperialInches:
    {
        double totalInches = value / kInchToMeter;
        return QString::number(totalInches, 'f', 2) + QStringLiteral("\"");
    }
    case UnitSystem::ImperialFeetInches:
    {
        double totalInches = value / kInchToMeter;
        int feet = static_cast<int>(std::floor(totalInches / 12.0));
        double inches = totalInches - feet * 12.0;
        return QStringLiteral("%1'%2\"").arg(feet).arg(QString::number(inches, 'f', 2));
    }
    }
    return QString();
}
