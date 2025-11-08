#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QLineEdit>
#include <QListView>
#include <QSettings>
#include <QTemporaryDir>
#include <QString>
#include <cmath>
#include <cassert>

#include "CameraController.h"
#include "MainWindow.h"
#include "GLViewport.h"
#include "Scene/SceneSettings.h"
#include "ui/CommandPaletteDialog.h"

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QTemporaryDir configDir;
    assert(configDir.isValid());

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, configDir.path());

    QApplication app(argc, argv);

    QSettings settings("FreeCrafter", "FreeCrafter");
    settings.clear();
    settings.sync();

    {
        MainWindow w;

        auto menus = w.menuBar()->actions();
        bool hasFile = false;
        bool hasEdit = false;
        for (QAction* act : menus) {
            if (act->text() == "&File") {
                hasFile = true;
                QMenu* fileMenu = act->menu();
                bool hasExit = false;
                for (QAction* fileAct : fileMenu->actions()) {
                    if (fileAct->text() == "Exit") {
                        hasExit = true;
                    }
                }
                assert(hasExit);
            } else if (act->text() == "&Edit") {
                hasEdit = true;
            }
        }
        assert(hasFile && hasEdit);

        QDockWidget* terminalDock = w.findChild<QDockWidget*>(QStringLiteral("TerminalDock"));
        assert(terminalDock);
        assert(!terminalDock->isVisible());

        QMetaObject::invokeMethod(&w, "toggleTerminalDock", Qt::DirectConnection);
        assert(terminalDock->isVisible());

        QMetaObject::invokeMethod(&w, "toggleTerminalDock", Qt::DirectConnection);
        assert(!terminalDock->isVisible());

        QMetaObject::invokeMethod(&w, "toggleTerminalDock", Qt::DirectConnection);
        assert(terminalDock->isVisible());

        GLViewport* viewport = w.findChild<GLViewport*>(QStringLiteral("MainViewport"));
        assert(viewport);
        Scene::Document* document = viewport->getDocument();
        assert(document);

        QAction* sectionPlanesAction = w.findChild<QAction*>(QStringLiteral("actionShowSectionPlanes"));
        QAction* sectionFillsAction = w.findChild<QAction*>(QStringLiteral("actionShowSectionFills"));
        QAction* guidesAction = w.findChild<QAction*>(QStringLiteral("actionShowGuides"));
        QAction* shadowsAction = w.findChild<QAction*>(QStringLiteral("actionEnableShadows"));
        QAction* perspectiveAction = w.findChild<QAction*>(QStringLiteral("actionPerspectiveProjection"));
        QAction* parallelAction = w.findChild<QAction*>(QStringLiteral("actionParallelProjection"));
        QAction* toggleProjectionAction = w.findChild<QAction*>(QStringLiteral("actionToggleProjection"));
        QAction* gridToggleAction = w.findChild<QAction*>(QStringLiteral("actionShowGrid"));
        assert(sectionPlanesAction && sectionFillsAction && guidesAction && shadowsAction);
        assert(perspectiveAction && parallelAction && toggleProjectionAction && gridToggleAction);

        assert(perspectiveAction->isChecked());
        assert(!parallelAction->isChecked());
        assert(!toggleProjectionAction->isChecked());
        assert(gridToggleAction->isChecked());
        assert(viewport->projectionMode() == CameraController::ProjectionMode::Perspective);
        assert(viewport->showGrid());

        parallelAction->trigger();
        assert(parallelAction->isChecked());
        assert(!perspectiveAction->isChecked());
        assert(toggleProjectionAction->isChecked());
        assert(viewport->projectionMode() == CameraController::ProjectionMode::Parallel);

        toggleProjectionAction->trigger();
        assert(!toggleProjectionAction->isChecked());
        assert(perspectiveAction->isChecked());
        assert(!parallelAction->isChecked());
        assert(viewport->projectionMode() == CameraController::ProjectionMode::Perspective);

        gridToggleAction->trigger();
        assert(!gridToggleAction->isChecked());
        assert(!viewport->showGrid());

        toggleProjectionAction->trigger();
        assert(toggleProjectionAction->isChecked());
        assert(parallelAction->isChecked());
        assert(!perspectiveAction->isChecked());
        assert(viewport->projectionMode() == CameraController::ProjectionMode::Parallel);

        assert(sectionPlanesAction->isChecked());
        sectionPlanesAction->trigger();
        assert(!document->settings().sectionPlanesVisible());

        assert(sectionFillsAction->isChecked());
        sectionFillsAction->trigger();
        assert(!document->settings().sectionFillsVisible());

        assert(guidesAction->isChecked());
        guidesAction->trigger();
        assert(!document->settings().guidesVisible());

        assert(!shadowsAction->isChecked());
        shadowsAction->trigger();
        assert(viewport->sunSettings().shadowsEnabled);

        bool ok = QMetaObject::invokeMethod(&w,
                                            "applyGridSettings",
                                            Qt::DirectConnection,
                                            Q_ARG(double, 2.5),
                                            Q_ARG(int, 6),
                                            Q_ARG(int, 80));
        assert(ok);
        Scene::SceneSettings::GridSettings grid = document->settings().grid();
        assert(std::fabs(grid.majorSpacing - 2.5f) < 1e-6f);
        assert(grid.minorDivisions == 6);
        assert(grid.majorExtent == 80);

        ok = QMetaObject::invokeMethod(&w,
                                       "applyShadowParameters",
                                       Qt::DirectConnection,
                                       Q_ARG(bool, true),
                                       Q_ARG(int, 2),
                                       Q_ARG(double, 55.0),
                                       Q_ARG(double, 0.002));
        assert(ok);
        Scene::SceneSettings::ShadowSettings shadow = document->settings().shadows();
        assert(shadow.enabled);
        assert(shadow.quality == Scene::SceneSettings::ShadowQuality::High);
        assert(std::fabs(shadow.strength - 0.55f) < 1e-6f);
        assert(std::fabs(shadow.bias - 0.002f) < 1e-6f);
        assert(viewport->sunSettings().shadowsEnabled);

        QMetaObject::invokeMethod(&w, "showCommandPalette", Qt::DirectConnection);
        app.processEvents();

        CommandPaletteDialog* paletteDialog = w.findChild<CommandPaletteDialog*>(QStringLiteral("CommandPaletteDialog"));
        assert(paletteDialog);

        QLineEdit* paletteSearch = paletteDialog->findChild<QLineEdit*>(QStringLiteral("CommandPaletteSearch"));
        QListView* paletteResults = paletteDialog->findChild<QListView*>(QStringLiteral("CommandPaletteResults"));
        assert(paletteSearch && paletteResults);
        assert(paletteResults->model());

        paletteSearch->setText(QStringLiteral("Line"));
        app.processEvents();
        assert(paletteResults->model()->rowCount() > 0);

        paletteDialog->activateCurrent();
        app.processEvents();

        assert(!paletteDialog->isVisible());

        QAction* lineToolAction = w.findChild<QAction*>(QStringLiteral("LineTool"));
        assert(lineToolAction && lineToolAction->isChecked());

        w.close();
        app.processEvents();
    }

    {
        MainWindow reopened;
        QDockWidget* terminalDock = reopened.findChild<QDockWidget*>(QStringLiteral("TerminalDock"));
        assert(terminalDock);
        assert(terminalDock->isVisible());

        QMetaObject::invokeMethod(&reopened, "toggleTerminalDock", Qt::DirectConnection);
        assert(!terminalDock->isVisible());

        GLViewport* reopenedViewport = reopened.findChild<GLViewport*>(QStringLiteral("MainViewport"));
        assert(reopenedViewport);
        Scene::Document* reopenedDocument = reopenedViewport->getDocument();
        assert(reopenedDocument);
        const Scene::SceneSettings& reopenedSettings = reopenedDocument->settings();
        assert(!reopenedSettings.sectionPlanesVisible());
        assert(!reopenedSettings.sectionFillsVisible());
        assert(!reopenedSettings.guidesVisible());
        Scene::SceneSettings::GridSettings reopenedGrid = reopenedSettings.grid();
        assert(std::fabs(reopenedGrid.majorSpacing - 2.5f) < 1e-6f);
        assert(reopenedGrid.minorDivisions == 6);
        assert(reopenedGrid.majorExtent == 80);
        Scene::SceneSettings::ShadowSettings reopenedShadow = reopenedSettings.shadows();
        assert(reopenedShadow.enabled);
        assert(reopenedShadow.quality == Scene::SceneSettings::ShadowQuality::High);
        assert(std::fabs(reopenedShadow.strength - 0.55f) < 1e-6f);
        assert(std::fabs(reopenedShadow.bias - 0.002f) < 1e-6f);
        assert(reopenedViewport->sunSettings().shadowsEnabled);

        QAction* reopenedPerspective = reopened.findChild<QAction*>(QStringLiteral("actionPerspectiveProjection"));
        QAction* reopenedParallel = reopened.findChild<QAction*>(QStringLiteral("actionParallelProjection"));
        QAction* reopenedToggleProjection = reopened.findChild<QAction*>(QStringLiteral("actionToggleProjection"));
        QAction* reopenedGridToggle = reopened.findChild<QAction*>(QStringLiteral("actionShowGrid"));
        assert(reopenedPerspective && reopenedParallel && reopenedToggleProjection && reopenedGridToggle);
        assert(!reopenedPerspective->isChecked());
        assert(reopenedParallel->isChecked());
        assert(reopenedToggleProjection->isChecked());
        assert(!reopenedGridToggle->isChecked());
        assert(reopenedViewport->projectionMode() == CameraController::ProjectionMode::Parallel);
        assert(!reopenedViewport->showGrid());

        QAction* reopenedPlanes = reopened.findChild<QAction*>(QStringLiteral("actionShowSectionPlanes"));
        QAction* reopenedFills = reopened.findChild<QAction*>(QStringLiteral("actionShowSectionFills"));
        QAction* reopenedGuides = reopened.findChild<QAction*>(QStringLiteral("actionShowGuides"));
        QAction* reopenedShadows = reopened.findChild<QAction*>(QStringLiteral("actionEnableShadows"));
        assert(reopenedPlanes && reopenedFills && reopenedGuides && reopenedShadows);
        assert(!reopenedPlanes->isChecked());
        assert(!reopenedFills->isChecked());
        assert(!reopenedGuides->isChecked());
        assert(reopenedShadows->isChecked());

        reopened.close();
        app.processEvents();
    }

    return 0;
}

