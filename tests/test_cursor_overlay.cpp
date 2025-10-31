#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QMouseEvent>
#include <QProcessEnvironment>
#include <QSettings>
#include <QTemporaryDir>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <cassert>

#include "GLViewport.h"
#include "Tools/ToolManager.h"

namespace {

void simulateMove(GLViewport& viewport, const QPointF& position)
{
    QMouseEvent event(QEvent::MouseMove, position, position, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&viewport, &event);
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QTemporaryDir configDir;
    assert(configDir.isValid());

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, configDir.path());

    QApplication app(argc, argv);

    GLViewport viewport;
    viewport.resize(640, 480);
    viewport.show();
    app.processEvents();

    ToolManager manager(viewport.getDocument(), viewport.getCamera(), nullptr);
    viewport.setToolManager(&manager);

    simulateMove(viewport, QPointF(200.0, 200.0));
    auto selectSnapshot = viewport.queryCursorOverlaySnapshot();
    assert(selectSnapshot.hasTool);
    assert(selectSnapshot.toolName == QStringLiteral("SmartSelectTool"));
    assert(selectSnapshot.descriptor.mode == Tool::CursorDescriptor::Mode::Pointer);
    assert(selectSnapshot.descriptor.showPickCircle);

    manager.activateTool("LineTool");
    app.processEvents();
    simulateMove(viewport, QPointF(260.0, 210.0));
    auto drawSnapshot = viewport.queryCursorOverlaySnapshot();
    assert(drawSnapshot.toolName == QStringLiteral("LineTool"));
    assert(drawSnapshot.descriptor.mode == Tool::CursorDescriptor::Mode::Draw);
    assert(drawSnapshot.descriptor.showCrosshair);
    assert(drawSnapshot.descriptor.showPickCircle);
    assert(!drawSnapshot.modifierHint.isEmpty());

    manager.activateTool("MoveTool");
    app.processEvents();
    simulateMove(viewport, QPointF(280.0, 215.0));
    auto moveSnapshot = viewport.queryCursorOverlaySnapshot();
    assert(moveSnapshot.toolName == QStringLiteral("MoveTool"));
    assert(moveSnapshot.descriptor.mode == Tool::CursorDescriptor::Mode::Move);
    assert(moveSnapshot.descriptor.showPickCircle);

    manager.activateTool("RotateTool");
    app.processEvents();
    simulateMove(viewport, QPointF(300.0, 220.0));
    auto rotateSnapshot = viewport.queryCursorOverlaySnapshot();
    assert(rotateSnapshot.toolName == QStringLiteral("RotateTool"));
    assert(rotateSnapshot.descriptor.mode == Tool::CursorDescriptor::Mode::Rotate);
    assert(rotateSnapshot.descriptor.showCrosshair);

    manager.activateTool("PaintBucket");
    app.processEvents();
    simulateMove(viewport, QPointF(320.0, 240.0));
    auto annotateSnapshot = viewport.queryCursorOverlaySnapshot();
    assert(annotateSnapshot.toolName == QStringLiteral("PaintBucket"));
    assert(annotateSnapshot.descriptor.mode == Tool::CursorDescriptor::Mode::Annotate);
    assert(annotateSnapshot.descriptor.showPickCircle);

    if (qEnvironmentVariableIsSet("FREECRAFTER_CAPTURE_CURSOR")) {
        viewport.update();
        app.processEvents();
        QImage capture = viewport.grabFramebuffer();
        QString path = qEnvironmentVariable("FREECRAFTER_CAPTURE_CURSOR");
        if (path.isEmpty())
            return 0;

        bool wantsBinary = path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive);

        if (wantsBinary) {
            capture.save(path);
        } else {
            QByteArray pngBytes;
            QBuffer buffer(&pngBytes);
            buffer.open(QIODevice::WriteOnly);
            capture.save(&buffer, "PNG");
            buffer.close();

            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                file.write(pngBytes.toBase64());
        }
    }

    return 0;
}
