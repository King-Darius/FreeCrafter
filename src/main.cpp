#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // Request a modern OpenGL context compatible with the renderer's shaders
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("FreeCrafter");
    QCoreApplication::setOrganizationDomain("freecrafter.io");
    QCoreApplication::setApplicationName("FreeCrafter");
    MainWindow w;
    w.show();
    return app.exec();
}
