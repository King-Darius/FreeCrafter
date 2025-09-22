#pragma once
#include <QMainWindow>
#include <memory>

class GLViewport;
class QAction;
class QLineEdit;
class QLabel;
class QComboBox;
class QCloseEvent;
class HotkeyManager;

#include <QByteArray>

#include "Tools/ToolManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
protected:
    void closeEvent(QCloseEvent* event) override;
private slots:
    void newFile();
    void openFile();
    void saveFile();
    void activateSelect();
    void activateSketch();
    void activateExtrude();
    void importHotkeyMap();
    void showHotkeyEditor();
    void applyMeasurementOverride();
    void unitChanged(int index);
private:
    enum class UnitSystem {
        MetricMeters = 0,
        ImperialFeetInches,
        ImperialInches
    };

    void createMenus();
    void createToolbars();
    void createDockPanels();
    void createStatusWidgets();
    void loadSettings();
    void saveSettings();
    void updateToolUi();
    void loadDefaultHotkeys();
    bool parseMeasurement(const QString& text, double& value) const;
    QString formatMeasurement(double value) const;

    GLViewport* viewport = nullptr;
    std::unique_ptr<ToolManager> toolManager;
    std::unique_ptr<HotkeyManager> hotkeyManager;
    QLineEdit* measurementBox = nullptr;
    QLabel* hintLabel = nullptr;
    QLabel* unitLabel = nullptr;
    QComboBox* unitSelector = nullptr;

    QAction* selectAction = nullptr;
    QAction* sketchAction = nullptr;
    QAction* extrudeAction = nullptr;
    QAction* importHotkeysAction = nullptr;
    QAction* hotkeyEditorAction = nullptr;
    QAction* newFileAction = nullptr;
    QAction* openFileAction = nullptr;
    QAction* saveFileAction = nullptr;
    QAction* zoomExtentsAction = nullptr;

    UnitSystem currentUnits = UnitSystem::MetricMeters;
};
