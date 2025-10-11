#include "SunModel.h"

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

constexpr float kMinValidAltitude = -5.0f;

float normalizeAzimuth(float azimuthDegrees)
{
    float azimuth = std::fmod(azimuthDegrees, 360.0f);
    if (azimuth < 0.0f)
        azimuth += 360.0f;
    return azimuth;
}

} // namespace

SunModel::Result SunModel::computeSunDirection(float altitudeDegrees, float azimuthDegrees)
{
    Result result;

    const float clampedAltitude = std::clamp(altitudeDegrees, -90.0f, 90.0f);
    const float normalizedAzimuth = normalizeAzimuth(azimuthDegrees);

    result.altitudeDegrees = clampedAltitude;
    result.azimuthDegrees = normalizedAzimuth;

    if (clampedAltitude <= kMinValidAltitude) {
        return result;

    const float altitudeRad = qDegreesToRadians(clampedAltitude);
    const float azimuthRad = qDegreesToRadians(normalizedAzimuth);

    const float horizontal = std::cos(altitudeRad);
    const float east = horizontal * std::sin(azimuthRad);
    const float north = horizontal * std::cos(azimuthRad);
    const float up = std::sin(altitudeRad);

    QVector3D direction(east, up, north);
    if (direction.isNull()) {
        return result;
    }

    direction.normalize();
    result.direction = direction;
    result.valid = true;
    return result;
}


    const double sunDeclination = std::asin(std::sin(obliqCorrRad) * std::sin(sunAppLongRad));



    const double varY = std::tan(obliqCorrRad / 2.0);

    const double varYSq = varY * varY;



    const double eqOfTime = 4.0 * radToDeg(varYSq * std::sin(2.0 * geomMeanLongSunRad)

                                           - 2.0 * eccentEarthOrbit * std::sin(geomMeanAnomSunRad)

                                           + 4.0 * eccentEarthOrbit * varYSq * std::sin(geomMeanAnomSunRad) * std::cos(2.0 * geomMeanLongSunRad)

                                           - 0.5 * varYSq * varYSq * std::sin(4.0 * geomMeanLongSunRad)

                                           - 1.25 * eccentEarthOrbit * eccentEarthOrbit * std::sin(2.0 * geomMeanAnomSunRad));



    const double timezoneHours = timezoneMinutes / 60.0;

    const double minutes = effectiveTime.hour() * 60.0 + effectiveTime.minute() + effectiveTime.second() / 60.0;

    double trueSolarTime = minutes + eqOfTime + 4.0 * longitudeDegrees - 60.0 * timezoneHours;

    trueSolarTime = std::fmod(trueSolarTime, 1440.0);

    if (trueSolarTime < 0.0)

        trueSolarTime += 1440.0;



    double hourAngle = trueSolarTime / 4.0 - 180.0;

    if (hourAngle < -180.0)

        hourAngle += 360.0;



    const double hourAngleRad = degToRad(hourAngle);

    const double latitudeRad = degToRad(latitudeDegrees);



    double cosZenith = std::sin(latitudeRad) * std::sin(sunDeclination) + std::cos(latitudeRad) * std::cos(sunDeclination) * std::cos(hourAngleRad);

    cosZenith = std::clamp(cosZenith, -1.0, 1.0);



    const double zenith = std::acos(cosZenith);

    const double altitude = kPi / 2.0 - zenith;



    double azimuth = std::atan2(std::sin(hourAngleRad), std::cos(hourAngleRad) * std::sin(latitudeRad) - std::tan(sunDeclination) * std::cos(latitudeRad));

    double azimuthDeg = radToDeg(azimuth) + 180.0;

    if (azimuthDeg < 0.0)

        azimuthDeg += 360.0;

    if (azimuthDeg >= 360.0)

        azimuthDeg -= 360.0;



    result.altitudeDegrees = static_cast<float>(radToDeg(altitude));

    result.azimuthDegrees = static_cast<float>(azimuthDeg);



    if (result.altitudeDegrees <= -0.5f) {

        result.valid = false;

        return result;

    }



    const double altitudeRad = degToRad(result.altitudeDegrees);

    const double azimuthRad = degToRad(result.azimuthDegrees);



    const double horizontal = std::cos(altitudeRad);

    const double east = horizontal * std::sin(azimuthRad);

    const double north = horizontal * std::cos(azimuthRad);

    const double up = std::sin(altitudeRad);



    result.direction = QVector3D(static_cast<float>(east), static_cast<float>(up), static_cast<float>(north));

    if (!result.direction.isNull()) {

        result.direction.normalize();

        result.valid = true;

    }



    return result;

}

