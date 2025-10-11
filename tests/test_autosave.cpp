#include "app/AutosaveManager.h"
#include "Scene/Document.h"
#include "GeometryKernel/Vector3.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QThread>

#include <cassert>
#include <vector>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QTemporaryDir tempDir;
    assert(tempDir.isValid());

    AutosaveManager manager(tempDir.path());
    Scene::Document document;
    manager.setDocument(&document);
    manager.setIntervalMinutes(0);
    manager.setRetentionCount(2);
    manager.setSourcePath(QStringLiteral("/tmp/sample.fcm"));

    manager.performAutosave();
    QThread::msleep(5);
    manager.performAutosave();
    QThread::msleep(5);
    manager.performAutosave();

    const auto snapshots = manager.autosaves();
    assert(snapshots.size() == 2);

    manager.clearAutosaves();
    assert(manager.autosaves().empty());

    manager.setRetentionCount(3);
    manager.performAutosave();
    assert(!manager.autosaves().empty());

    // Prepare a document with geometry and create an autosave
    Scene::Document restoreSource;
    auto* line = restoreSource.geometry().addCurve({ Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f } });
    assert(line);
    restoreSource.ensureObjectForGeometry(line, "Edge");

    AutosaveManager restoreManager(tempDir.path());
    restoreManager.setDocument(&restoreSource);
    restoreManager.setIntervalMinutes(0);
    restoreManager.setSourcePath(QStringLiteral("/tmp/restore.fcm"));
    restoreManager.performAutosave();

    assert(!restoreSource.geometry().getObjects().empty());
    restoreSource.reset();
    assert(restoreSource.geometry().getObjects().empty());

    const bool restored = restoreManager.restoreLatest(&restoreSource);
    assert(restored);
    assert(!restoreSource.geometry().getObjects().empty());

    restoreManager.clearAutosaves();

    return 0;
}
