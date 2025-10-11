#include "app/AutosaveManager.h"

#include "Scene/Document.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>

#include <algorithm>

namespace {
constexpr int kDefaultIntervalMinutes = 5;
constexpr int kDefaultRetentionCount = 5;
}

AutosaveManager::AutosaveManager(QObject* parent)
    : QObject(parent)
{
    timer_.setTimerType(Qt::VeryCoarseTimer);
    initializeStorageDirectory();
    connect(&timer_, &QTimer::timeout, this, &AutosaveManager::performAutosave);
    setIntervalMinutes(kDefaultIntervalMinutes);
    setRetentionCount(kDefaultRetentionCount);
    prefixCache_ = makePrefix(sourcePath_);
}

AutosaveManager::AutosaveManager(const QString& storageRoot, QObject* parent)
    : QObject(parent)
{
    timer_.setTimerType(Qt::VeryCoarseTimer);
    initializeStorageDirectory(storageRoot);
    connect(&timer_, &QTimer::timeout, this, &AutosaveManager::performAutosave);
    setIntervalMinutes(kDefaultIntervalMinutes);
    setRetentionCount(kDefaultRetentionCount);
    prefixCache_ = makePrefix(sourcePath_);
}

void AutosaveManager::initializeStorageDirectory(const QString& rootOverride)
{
    QString base = rootOverride;
    if (base.isEmpty())
        base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty())
        base = QDir::homePath();

    QDir dir(base);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    if (!dir.mkpath(QStringLiteral("autosave"))) {
        // Directory may already exist; ensure we still record the path.
        if (!dir.exists(QStringLiteral("autosave")))
            return;
    }

    autosaveDir_ = QDir::cleanPath(dir.filePath(QStringLiteral("autosave")));
}

void AutosaveManager::setDocument(Scene::Document* document)
{
    if (document_ == document)
        return;
    document_ = document;
    updateTimer();
}

void AutosaveManager::setSourcePath(const QString& path)
{
    if (sourcePath_ == path)
        return;
    sourcePath_ = path;
    prefixCache_ = makePrefix(sourcePath_);
}

void AutosaveManager::setIntervalMinutes(int minutes)
{
    intervalMinutes_ = std::max(0, minutes);
    updateTimer();
}

void AutosaveManager::setRetentionCount(int count)
{
    retentionCount_ = std::max(1, count);
}

void AutosaveManager::updateTimer()
{
    if (intervalMinutes_ <= 0 || !document_) {
        timer_.stop();
        return;
    }
    const int intervalMs = intervalMinutes_ * 60 * 1000;
    if (timer_.interval() != intervalMs || !timer_.isActive())
        timer_.start(intervalMs);
}

QString AutosaveManager::timestampedFilename() const
{
    if (prefixCache_.isEmpty())
        const_cast<AutosaveManager*>(this)->prefixCache_ = makePrefix(sourcePath_);

    const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmsszzz"));
    return QStringLiteral("%1_%2.fcm").arg(prefixCache_, stamp);
}

void AutosaveManager::performAutosave()
{
    if (!document_ || autosaveDir_.isEmpty())
        return;

    QDir dir(autosaveDir_);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    const QString filename = timestampedFilename();
    const QString filePath = dir.filePath(filename);

    if (!document_->saveToFile(filePath.toStdString())) {
        QFile::remove(filePath);
        return;
    }

    const auto files = autosaveFileInfos(prefixCache_);
    enforceRetention(files);
}

QVector<QFileInfo> AutosaveManager::autosaveFileInfos(const QString& prefix) const
{
    QVector<QFileInfo> result;
    if (autosaveDir_.isEmpty() || prefix.isEmpty())
        return result;

    QDir dir(autosaveDir_);
    if (!dir.exists())
        return result;

    const QString pattern = QStringLiteral("%1_*.fcm").arg(prefix);
    const QFileInfoList entries = dir.entryInfoList(QStringList{ pattern }, QDir::Files, QDir::Time);
    result.reserve(entries.size());
    for (const QFileInfo& info : entries)
        result.push_back(info);
    return result;
}

void AutosaveManager::enforceRetention(const QVector<QFileInfo>& files)
{
    if (retentionCount_ <= 0)
        return;

    for (int i = retentionCount_; i < files.size(); ++i)
        QFile::remove(files[i].absoluteFilePath());
}

QVector<AutosaveManager::AutosaveInfo> AutosaveManager::autosaves() const
{
    QVector<AutosaveInfo> list;
    const auto files = autosaveFileInfos(prefixCache_);
    list.reserve(files.size());
    for (const QFileInfo& info : files) {
        AutosaveInfo entry;
        entry.filePath = info.absoluteFilePath();
        entry.prefix = prefixCache_;
        entry.timestamp = info.lastModified().toUTC();
        list.push_back(entry);
    }
    return list;
}

std::optional<AutosaveManager::AutosaveInfo> AutosaveManager::latestAutosave() const
{
    const auto files = autosaveFileInfos(prefixCache_);
    if (files.isEmpty())
        return std::nullopt;

    AutosaveInfo entry;
    entry.filePath = files.front().absoluteFilePath();
    entry.prefix = prefixCache_;
    entry.timestamp = files.front().lastModified().toUTC();
    return entry;
}

bool AutosaveManager::restoreLatest(Scene::Document* document)
{
    Scene::Document* target = document ? document : document_;
    if (!target)
        return false;

    const auto files = autosaveFileInfos(prefixCache_);
    if (files.isEmpty())
        return false;

    const QString path = files.front().absoluteFilePath();
    return target->loadFromFile(path.toStdString());
}

void AutosaveManager::clearAutosaves()
{
    clearAutosavesForPrefix(prefixCache_);
}

void AutosaveManager::clearAutosavesForPrefix(const QString& prefix)
{
    if (autosaveDir_.isEmpty() || prefix.isEmpty())
        return;

    QDir dir(autosaveDir_);
    if (!dir.exists())
        return;

    const QString pattern = QStringLiteral("%1_*.fcm").arg(prefix);
    const QFileInfoList entries = dir.entryInfoList(QStringList{ pattern }, QDir::Files);
    for (const QFileInfo& info : entries)
        QFile::remove(info.absoluteFilePath());
}

void AutosaveManager::shutdown()
{
    timer_.stop();
}

QString AutosaveManager::makePrefix(const QString& path) const
{
    QString baseName;
    if (!path.isEmpty()) {
        const QFileInfo info(path);
        baseName = info.completeBaseName();
        if (baseName.isEmpty())
            baseName = info.fileName();
    } else {
        baseName = QStringLiteral("untitled");
    }

    baseName = sanitizeName(baseName);
    if (baseName.isEmpty())
        baseName = QStringLiteral("document");

    const QByteArray hash = QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString hashFragment = QString::fromLatin1(hash.left(8));
    return QStringLiteral("%1_%2").arg(baseName.left(32), hashFragment);
}

QString AutosaveManager::sanitizeName(const QString& value)
{
    QString cleaned;
    cleaned.reserve(value.size());
    for (const QChar c : value) {
        if (c.isLetterOrNumber()) {
            cleaned.append(c);
        } else if (c == QLatin1Char('-') || c == QLatin1Char('_')) {
            cleaned.append(QChar::fromLatin1('_'));
        } else if (c.isSpace()) {
            cleaned.append(QChar::fromLatin1('_'));
        }
    }
    cleaned.replace(QRegularExpression(QStringLiteral("_+")), QStringLiteral("_"));
    while (cleaned.startsWith(QLatin1Char('_')))
        cleaned.remove(0, 1);
    while (cleaned.endsWith(QLatin1Char('_')))
        cleaned.chop(1);
    return cleaned.toLower();
}

