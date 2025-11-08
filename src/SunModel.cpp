#include "SunModel.h"

#include <QtMath>
#include <algorithm>

namespace {
constexpr float kMinValidAltitude = -5.0f;

float normalizeAzimuth(float azimuthDegrees)
{
    float wrapped = std::fmod(azimuthDegrees, 360.0f);
    if (wrapped < 0.0f)
        wrapped += 360.0f;
    return wrapped;
}
} // namespace

SunModel::Result SunModel::computeSunDirection(float altitudeDegrees, float azimuthDegrees)
{
    Result result;

    const float clampedAltitude = std::clamp(altitudeDegrees, -90.0f, 90.0f);
    const float normalizedAzimuth = normalizeAzimuth(azimuthDegrees);

    result.altitudeDegrees = clampedAltitude;
    result.azimuthDegrees = normalizedAzimuth;

    if (clampedAltitude <= kMinValidAltitude)
        return result;

    const float altitudeRad = qDegreesToRadians(clampedAltitude);
    const float azimuthRad = qDegreesToRadians(normalizedAzimuth);

    const float horizontal = std::cos(altitudeRad);
    const float east = horizontal * std::sin(azimuthRad);
    const float north = horizontal * std::cos(azimuthRad);
    const float up = std::sin(altitudeRad);

    QVector3D direction(east, up, north);
    if (direction.isNull())
        return result;

    direction.normalize();
    result.direction = direction;
    result.valid = true;
    return result;
}


    if (!result.direction.isNull()) {

        result.direction.normalize();

        result.valid = true;

    }



    return result;

}

