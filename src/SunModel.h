#pragma once

#include <QVector3D>

class SunModel
{
public:
    struct Result {
        QVector3D direction;
        float altitudeDegrees = 0.0f;
        float azimuthDegrees = 0.0f;
        bool valid = false;
    };

    static Result computeSunDirection(float altitudeDegrees, float azimuthDegrees);
};
