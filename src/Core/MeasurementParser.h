#pragma once

#include <QString>

#include "Tools/Tool.h"

namespace Measurement {

struct ParseResult {
    bool ok = false;
    double value = 0.0;
    QString display;
    QString error;
};

ParseResult parseImperialDistance(const QString& raw);
ParseResult parseMetricDistance(const QString& raw);
ParseResult parseDistance(const QString& raw, const QString& unitSystem);
ParseResult parseDistanceFlexible(const QString& raw);
ParseResult parseAngle(const QString& raw);
ParseResult parseScale(const QString& raw);
ParseResult parseCount(const QString& raw);
ParseResult parseMeasurement(const QString& raw, const QString& unitSystem, Tool::MeasurementKind kind);
QString measurementHintForKind(Tool::MeasurementKind kind);

} // namespace Measurement

