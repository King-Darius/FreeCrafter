#include <cassert>
#include <cmath>

#include "SunModel.h"

namespace {
constexpr float kEpsilon = 1e-4f;
}

int main()
{
    const auto highSun = SunModel::computeSunDirection(45.0f, 90.0f);
    assert(highSun.valid);
    assert(std::fabs(highSun.altitudeDegrees - 45.0f) < kEpsilon);
    assert(std::fabs(highSun.azimuthDegrees - 90.0f) < kEpsilon);
    assert(std::fabs(highSun.direction.length() - 1.0f) < kEpsilon);
    assert(highSun.direction.x() > 0.7f);
    assert(highSun.direction.y() > 0.7f);
    assert(std::fabs(highSun.direction.z()) < kEpsilon);

    const auto wrapped = SunModel::computeSunDirection(10.0f, -30.0f);
    assert(wrapped.valid);
    assert(std::fabs(wrapped.altitudeDegrees - 10.0f) < kEpsilon);
    assert(std::fabs(wrapped.azimuthDegrees - 330.0f) < kEpsilon);

    const auto belowHorizon = SunModel::computeSunDirection(-15.0f, 120.0f);
    assert(!belowHorizon.valid);
    assert(std::fabs(belowHorizon.altitudeDegrees - (-15.0f)) < kEpsilon);
    assert(std::fabs(belowHorizon.azimuthDegrees - 120.0f) < kEpsilon);
    assert(belowHorizon.direction.isNull());

    const auto clampTest = SunModel::computeSunDirection(120.0f, 725.0f);
    assert(std::fabs(clampTest.altitudeDegrees - 90.0f) < kEpsilon);
    assert(std::fabs(clampTest.azimuthDegrees - 5.0f) < kEpsilon);

    return 0;
}
