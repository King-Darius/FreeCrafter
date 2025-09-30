#include "SunModel.h"

#include <QDateTime>
#include <QTimeZone>
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace {
constexpr double degToRad(double deg) { return deg * M_PI / 180.0; }
constexpr double radToDeg(double rad) { return rad * 180.0 / M_PI; }

double wrapDegrees(double value)
{
    double result = std::fmod(value, 360.0);
    if (result < 0.0)
        result += 360.0;
    return result;
}

} // namespace

SunModel::Result SunModel::computeSunDirection(const QDate& date,
                                               const QTime& time,
                                               double latitudeDegrees,
                                               double longitudeDegrees,
                                               int timezoneMinutes)
{
    Result result;
    if (!date.isValid()) {
        return result;
    }

    QTime effectiveTime = time.isValid() ? time : QTime(12, 0, 0);
    QTimeZone zone(timezoneMinutes * 60);
    QDateTime local(date, effectiveTime, zone);
    if (!local.isValid()) {
        local = QDateTime(date, effectiveTime, Qt::UTC);
        local = local.addSecs(-timezoneMinutes * 60);
    }

    QDateTime utc = local.toUTC();
    const double daySeconds = utc.time().msecsSinceStartOfDay() / 1000.0;
    const double julianDay = utc.date().toJulianDay() + daySeconds / 86400.0;
    const double julianCentury = (julianDay - 2451545.0) / 36525.0;

    const double geomMeanLongSun = wrapDegrees(280.46646 + julianCentury * (36000.76983 + julianCentury * 0.0003032));
    const double geomMeanAnomSun = 357.52911 + julianCentury * (35999.05029 - 0.0001537 * julianCentury);
    const double eccentEarthOrbit = 0.016708634 - julianCentury * (0.000042037 + 0.0000001267 * julianCentury);

    const double geomMeanLongSunRad = degToRad(geomMeanLongSun);
    const double geomMeanAnomSunRad = degToRad(geomMeanAnomSun);

    const double sunEqOfCenter = std::sin(geomMeanAnomSunRad) * (1.914602 - julianCentury * (0.004817 + 0.000014 * julianCentury))
        + std::sin(2.0 * geomMeanAnomSunRad) * (0.019993 - 0.000101 * julianCentury)
        + std::sin(3.0 * geomMeanAnomSunRad) * 0.000289;

    const double sunTrueLong = geomMeanLongSun + sunEqOfCenter;
    const double sunAppLong = sunTrueLong - 0.00569 - 0.00478 * std::sin(degToRad(125.04 - 1934.136 * julianCentury));

    const double meanObliqEcliptic = 23.0 + (26.0 + ((21.448 - julianCentury * (46.815 + julianCentury * (0.00059 - julianCentury * 0.001813))) / 60.0)) / 60.0;
    const double obliqCorr = meanObliqEcliptic + 0.00256 * std::cos(degToRad(125.04 - 1934.136 * julianCentury));

    const double obliqCorrRad = degToRad(obliqCorr);
    const double sunAppLongRad = degToRad(sunAppLong);
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
    const double altitude = M_PI / 2.0 - zenith;

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
