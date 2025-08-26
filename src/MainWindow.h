#pragma once
#include <QMainWindow>
#include <memory>

class GLViewport;
class QAction;
class QLineEdit;
class QLabel;

#include "Tools/ToolManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
private slots:
    void newFile();
    void openFile();
    void saveFile();
    void activateSelect();
    void activateSketch();
    void activateExtrude();
private:
    void createMenus();
    void createToolbars();
    void createDockPanels();

    GLViewport* viewport = nullptr;
    std::unique_ptr<ToolManager> toolManager;
    QLineEdit* measurementBox = nullptr;
    QLabel* hintLabel = nullptr;

    QAction* selectAction = nullptr;
    QAction* sketchAction = nullptr;
    QAction* extrudeAction = nullptr;
};
