#include "SunSettings.h"

#include <QDateTime>
#include <QtGlobal>
#include <QtMath>

SunSettings::SunSettings()
{
    const QDateTime now = QDateTime::currentDateTime();
    date = now.date();
    time = now.time();
    latitude = 37.7749; // default to San Francisco
    longitude = -122.4194;
    timezoneMinutes = now.offsetFromUtc() / 60;
    daylightSaving = false;
    shadowsEnabled = false;
    shadowQuality = ShadowQuality::Medium;
    shadowStrength = 0.65f;
    shadowBias = 0.0035f;
}

int SunSettings::effectiveTimezoneMinutes() const
{
    return timezoneMinutes + (daylightSaving ? 60 : 0);
}

void SunSettings::save(QSettings& settings, const QString& groupName) const
{
    settings.beginGroup(groupName);
    settings.setValue(QStringLiteral("date"), date);
    settings.setValue(QStringLiteral("time"), time);
    settings.setValue(QStringLiteral("latitude"), latitude);
    settings.setValue(QStringLiteral("longitude"), longitude);
    settings.setValue(QStringLiteral("timezoneMinutes"), timezoneMinutes);
    settings.setValue(QStringLiteral("daylightSaving"), daylightSaving);
    settings.setValue(QStringLiteral("shadowsEnabled"), shadowsEnabled);
    settings.setValue(QStringLiteral("shadowQuality"), static_cast<int>(shadowQuality));
    settings.setValue(QStringLiteral("shadowStrength"), static_cast<double>(shadowStrength));
    settings.setValue(QStringLiteral("shadowBias"), static_cast<double>(shadowBias));
    settings.endGroup();
}

void SunSettings::load(QSettings& settings, const QString& groupName)
{
    settings.beginGroup(groupName);
    if (settings.contains(QStringLiteral("date")))
        date = settings.value(QStringLiteral("date"), date).toDate();
    if (settings.contains(QStringLiteral("time")))
        time = settings.value(QStringLiteral("time"), time).toTime();
    latitude = settings.value(QStringLiteral("latitude"), latitude).toDouble();
    longitude = settings.value(QStringLiteral("longitude"), longitude).toDouble();
    timezoneMinutes = settings.value(QStringLiteral("timezoneMinutes"), timezoneMinutes).toInt();
    daylightSaving = settings.value(QStringLiteral("daylightSaving"), daylightSaving).toBool();
    shadowsEnabled = settings.value(QStringLiteral("shadowsEnabled"), shadowsEnabled).toBool();
    const int quality = settings.value(QStringLiteral("shadowQuality"), static_cast<int>(shadowQuality)).toInt();
    if (quality >= static_cast<int>(ShadowQuality::Low) && quality <= static_cast<int>(ShadowQuality::High)) {
        shadowQuality = static_cast<ShadowQuality>(quality);
    }
    shadowStrength = static_cast<float>(settings.value(QStringLiteral("shadowStrength"), static_cast<double>(shadowStrength)).toDouble());
    shadowStrength = qBound(0.0f, shadowStrength, 1.0f);
    shadowBias = static_cast<float>(settings.value(QStringLiteral("shadowBias"), static_cast<double>(shadowBias)).toDouble());
    settings.endGroup();
}
