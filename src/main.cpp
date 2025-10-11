#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QStringList>
#include <QSurfaceFormat>
#include <QResource>

int main(int argc, char* argv[])
{
    // Request a modern OpenGL context compatible with the renderer's shaders
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(fmt);

    Q_INIT_RESOURCE(resources);

    QApplication app(argc, argv);

    const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/DejaVuSans.ttf"));
    if (fontId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            QFont uiFont(families.front());
            uiFont.setStyleStrategy(QFont::PreferAntialias);
            app.setFont(uiFont);
        }
    }

    QCoreApplication::setOrganizationName("FreeCrafter");
    QCoreApplication::setOrganizationDomain("freecrafter.io");
    QCoreApplication::setApplicationName("FreeCrafter");
    MainWindow w;
    w.show();
    return app.exec();
}
