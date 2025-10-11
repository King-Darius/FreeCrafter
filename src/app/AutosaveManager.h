#pragma once

#include <QObject>
#include <QDateTime>
#include <QFileInfo>
#include <QTimer>
#include <QVector>

#include <optional>

namespace Scene {
class Document;
}

class AutosaveManager : public QObject {
    Q_OBJECT
public:
    struct AutosaveInfo {
        QString filePath;
        QString prefix;
        QDateTime timestamp;
    };

    explicit AutosaveManager(QObject* parent = nullptr);
    explicit AutosaveManager(const QString& storageRoot, QObject* parent = nullptr);

    void setDocument(Scene::Document* document);
    Scene::Document* document() const { return document_; }

    void setSourcePath(const QString& path);
    const QString& sourcePath() const { return sourcePath_; }

    void setIntervalMinutes(int minutes);
    int intervalMinutes() const { return intervalMinutes_; }

    void setRetentionCount(int count);
    int retentionCount() const { return retentionCount_; }

    void performAutosave();
    QVector<AutosaveInfo> autosaves() const;
    std::optional<AutosaveInfo> latestAutosave() const;
    bool restoreLatest(Scene::Document* document = nullptr);

    void clearAutosaves();
    void clearAutosavesForPrefix(const QString& prefix);

    QString autosaveDirectory() const { return autosaveDir_; }
    QString currentPrefix() const { return prefixCache_; }

    void shutdown();

private:
    void initializeStorageDirectory(const QString& rootOverride = QString());
    void updateTimer();
    QString makePrefix(const QString& path) const;
    QString timestampedFilename() const;
    QVector<QFileInfo> autosaveFileInfos(const QString& prefix) const;
    void enforceRetention(const QVector<QFileInfo>& files);
    static QString sanitizeName(const QString& value);

    Scene::Document* document_ = nullptr;
    QString sourcePath_;
    QString autosaveDir_;
    QString prefixCache_;
    int intervalMinutes_ = 5;
    int retentionCount_ = 5;
    QTimer timer_;
};

