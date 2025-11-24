#include "MainWindow.h"
#include "DebugSimpleViewport.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QStringList>
#include <QSurfaceFormat>
#include <QResource>
#include <QMainWindow>

int main(int argc, char* argv[])
{
    // Request a robust default OpenGL format without over-constraining the
    // profile or version. Some drivers silently fail to create a 3.3 core
    // context; allowing Qt to negotiate the best available profile makes the
    // viewport more resilient across platforms/GPUs.
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSamples(4);
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
    if (qEnvironmentVariableIsSet("FREECRAFTER_DEBUG_VIEWPORT")) {
        QMainWindow debugWindow;
        debugWindow.setWindowTitle(QStringLiteral("FreeCrafter Debug Viewport"));
        debugWindow.resize(960, 720);
        debugWindow.setCentralWidget(new DebugSimpleViewport(&debugWindow));
        debugWindow.show();
        return app.exec();
    }

    MainWindow w;
    w.show();
    return app.exec();
}
