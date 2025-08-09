#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QByteArray>
#include <cassert>

#include "MainWindow.h"

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

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
    return 0;
}

