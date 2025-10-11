#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QString>
#include <cassert>

#include "MainWindow.h"

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

        reopened.close();
        app.processEvents();
    }

    return 0;
}

