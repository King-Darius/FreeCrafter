#include <QApplication>
#include <QByteArray>
#include <QMetaObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QString>
#include <QUndoCommand>
#include <QUndoStack>
#include <QUndoView>

#include <cassert>

#include "GLViewport.h"
#include "MainWindow.h"
#include "Scene/Document.h"

int main(int argc, char** argv)
{
    // Force Qt to run headless so the test works in CI environments.
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

    MainWindow window;

    auto* historyView = window.findChild<QUndoView*>();
    assert(historyView != nullptr);
    QUndoStack* undoStack = historyView->stack();
    assert(undoStack != nullptr);

    // Seed the stack with an undoable action to confirm clearing works.
    undoStack->push(new QUndoCommand(QStringLiteral("Test Command")));
    assert(undoStack->canUndo());

    QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection);
    app.processEvents();
    assert(!undoStack->canUndo());

    // Re-seed after newFile() to verify openFileFromPath() also clears history.
    undoStack->push(new QUndoCommand(QStringLiteral("Test Command 2")));
    assert(undoStack->canUndo());

    GLViewport* viewport = window.findChild<GLViewport*>(QStringLiteral("MainViewport"));
    assert(viewport != nullptr);

    Scene::Document* document = viewport->getDocument();
    assert(document != nullptr);

    QTemporaryDir modelDir;
    assert(modelDir.isValid());

    const QString modelPath = modelDir.filePath(QStringLiteral("sample.fcm"));
    assert(document->saveToFile(modelPath.toStdString()));

    bool opened = false;
    QMetaObject::invokeMethod(&window,
                              "openFileFromPath",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(bool, opened),
                              Q_ARG(QString, modelPath));
    assert(opened);
    app.processEvents();
    assert(!undoStack->canUndo());

    return 0;
}
