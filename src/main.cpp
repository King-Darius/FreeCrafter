#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // Request compatibility profile for fixed-function calls used in the starter renderer
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setVersion(2, 1);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("FreeCrafter");
    QCoreApplication::setOrganizationDomain("freecrafter.io");
    QCoreApplication::setApplicationName("FreeCrafter");
    MainWindow w;
    w.show();
    return app.exec();
}
