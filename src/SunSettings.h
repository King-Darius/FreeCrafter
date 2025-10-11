#pragma once

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

    float elevationDegrees;
    float azimuthDegrees;
    bool shadowsEnabled;
    ShadowQuality shadowQuality;
    float shadowStrength; // 0-1 range
    float shadowBias;

    void save(QSettings& settings, const QString& groupName) const;
    void load(QSettings& settings, const QString& groupName);
};

