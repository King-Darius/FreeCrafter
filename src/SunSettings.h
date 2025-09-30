#pragma once

#include <QDate>
#include <QTime>
#include <QSettings>
#include <QString>

class SunSettings
{
public:
    enum class ShadowQuality {
        Low,
        Medium,
        High
    };

    SunSettings();

    QDate date;
    QTime time;
    double latitude;
    double longitude;
    int timezoneMinutes;
    bool daylightSaving;
    bool shadowsEnabled;
    ShadowQuality shadowQuality;
    float shadowStrength; // 0-1 range
    float shadowBias;

    int effectiveTimezoneMinutes() const;

    void save(QSettings& settings, const QString& groupName) const;
    void load(QSettings& settings, const QString& groupName);
};

