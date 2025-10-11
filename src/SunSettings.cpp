#include "SunSettings.h"

#include <QtGlobal>
#include <QtMath>

SunSettings::SunSettings()
{
    elevationDegrees = 45.0f;
    azimuthDegrees = 135.0f;
    shadowsEnabled = false;
    shadowQuality = ShadowQuality::Medium;
    shadowStrength = 0.65f;
    shadowBias = 0.0035f;
}

void SunSettings::save(QSettings& settings, const QString& groupName) const
{
    settings.beginGroup(groupName);
    settings.setValue(QStringLiteral("elevation"), static_cast<double>(elevationDegrees));
    settings.setValue(QStringLiteral("azimuth"), static_cast<double>(azimuthDegrees));
    settings.setValue(QStringLiteral("shadowsEnabled"), shadowsEnabled);
    settings.setValue(QStringLiteral("shadowQuality"), static_cast<int>(shadowQuality));
    settings.setValue(QStringLiteral("shadowStrength"), static_cast<double>(shadowStrength));
    settings.setValue(QStringLiteral("shadowBias"), static_cast<double>(shadowBias));
    settings.endGroup();
}

void SunSettings::load(QSettings& settings, const QString& groupName)
{
    settings.beginGroup(groupName);
    elevationDegrees = static_cast<float>(settings.value(QStringLiteral("elevation"), static_cast<double>(elevationDegrees)).toDouble());
    elevationDegrees = qBound(-90.0f, elevationDegrees, 90.0f);
    azimuthDegrees = static_cast<float>(settings.value(QStringLiteral("azimuth"), static_cast<double>(azimuthDegrees)).toDouble());
    while (azimuthDegrees < 0.0f)
        azimuthDegrees += 360.0f;
    while (azimuthDegrees >= 360.0f)
        azimuthDegrees -= 360.0f;
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
