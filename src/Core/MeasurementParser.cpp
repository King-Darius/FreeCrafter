#include "MeasurementParser.h"

#include <QObject>

namespace {

constexpr double kPi = 3.14159265358979323846;

QString removeWhitespace(QString text)
{
    text.remove(QChar(' '));
    text.remove(QChar('\t'));
    text.remove(QChar('\n'));
    text.remove(QChar('\r'));
    return text;
}

bool looksImperial(const QString& raw)
{
    const QString lower = raw.toLower();
    return lower.contains(QLatin1Char('\'')) || lower.contains(QLatin1Char('"')) ||
        lower.contains(QStringLiteral("ft")) || lower.contains(QStringLiteral("in"));
}

} // namespace

namespace Measurement {

ParseResult parseImperialDistance(const QString& raw)
{
    ParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty measurement");
        return result;
    }

    bool negative = false;
    if (sanitized.startsWith(QLatin1Char('+')) || sanitized.startsWith(QLatin1Char('-'))) {
        negative = sanitized.startsWith(QLatin1Char('-'));
        sanitized = sanitized.mid(1);
    }

    sanitized.replace(QStringLiteral("ft"), QStringLiteral("'"), Qt::CaseInsensitive);
    sanitized.replace(QStringLiteral("in"), QStringLiteral("\""), Qt::CaseInsensitive);

    double feet = 0.0;
    double inches = 0.0;

    const int footIndex = sanitized.indexOf(QLatin1Char('\''));
    if (footIndex >= 0) {
        const QString footPart = sanitized.left(footIndex);
        if (!footPart.isEmpty()) {
            bool ok = false;
            feet = footPart.toDouble(&ok);
            if (!ok) {
                result.error = QObject::tr("invalid feet value");
                return result;
            }
        }
        sanitized = sanitized.mid(footIndex + 1);
    }

    const int inchIndex = sanitized.indexOf(QLatin1Char('"'));
    if (inchIndex >= 0) {
        const QString inchPart = sanitized.left(inchIndex);
        if (!inchPart.isEmpty()) {
            bool ok = false;
            inches = inchPart.toDouble(&ok);
            if (!ok) {
                result.error = QObject::tr("invalid inches value");
                return result;
            }
        }
        sanitized = sanitized.mid(inchIndex + 1);
    } else if (!sanitized.isEmpty()) {
        bool ok = false;
        if (footIndex >= 0) {
            inches = sanitized.toDouble(&ok);
        } else {
            feet = sanitized.toDouble(&ok);
        }
        if (!ok) {
            result.error = QObject::tr("invalid measurement");
            return result;
        }
        sanitized.clear();
    }

    if (!sanitized.isEmpty()) {
        result.error = QObject::tr("unexpected trailing characters");
        return result;
    }

    double totalMeters = (feet * 12.0 + inches) * 0.0254;
    if (negative)
        totalMeters = -totalMeters;

    result.ok = true;
    result.value = totalMeters;
    result.display = raw.trimmed();
    if (result.display.isEmpty())
        result.display = QObject::tr("%1 m").arg(totalMeters, 0, 'f', 3);
    return result;
}

ParseResult parseMetricDistance(const QString& raw)
{
    ParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty measurement");
        return result;
    }

    QString lower = sanitized.toLower();
    double multiplier = 1.0;
    if (lower.endsWith(QStringLiteral("mm"))) {
        multiplier = 0.001;
        sanitized.chop(2);
    } else if (lower.endsWith(QStringLiteral("cm"))) {
        multiplier = 0.01;
        sanitized.chop(2);
    } else if (lower.endsWith(QStringLiteral("km"))) {
        multiplier = 1000.0;
        sanitized.chop(2);
    } else if (lower.endsWith(QStringLiteral("m"))) {
        multiplier = 1.0;
        sanitized.chop(1);
    }

    if (sanitized.isEmpty()) {
        result.error = QObject::tr("invalid measurement");
        return result;
    }

    bool ok = false;
    double magnitude = sanitized.toDouble(&ok);
    if (!ok) {
        result.error = QObject::tr("invalid measurement");
        return result;
    }

    result.ok = true;
    result.value = magnitude * multiplier;
    result.display = raw.trimmed();
    if (result.display.isEmpty())
        result.display = QObject::tr("%1 m").arg(result.value, 0, 'f', 3);
    return result;
}

ParseResult parseDistance(const QString& raw, const QString& unitSystem)
{
    if (unitSystem == QLatin1String("imperial"))
        return parseImperialDistance(raw);
    return parseMetricDistance(raw);
}

ParseResult parseDistanceFlexible(const QString& raw)
{
    if (looksImperial(raw))
        return parseImperialDistance(raw);
    return parseMetricDistance(raw);
}

ParseResult parseAngle(const QString& raw)
{
    ParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty angle");
        return result;
    }

    QString lower = sanitized.toLower();
    double multiplier = kPi / 180.0;
    if (lower.endsWith(QStringLiteral("rad"))) {
        multiplier = 1.0;
        sanitized.chop(3);
    } else if (lower.endsWith(QStringLiteral("deg"))) {
        sanitized.chop(3);
    } else if (sanitized.endsWith(QChar(0x00B0))) {
        sanitized.chop(1);
    }

    if (sanitized.isEmpty()) {
        result.error = QObject::tr("invalid angle");
        return result;
    }

    bool ok = false;
    double magnitude = sanitized.toDouble(&ok);
    if (!ok) {
        result.error = QObject::tr("invalid angle");
        return result;
    }

    result.ok = true;
    result.value = magnitude * multiplier;
    result.display = raw.trimmed();
    if (result.display.isEmpty())
        result.display = QObject::tr("%1°").arg(magnitude, 0, 'f', 2);
    return result;
}

ParseResult parseScale(const QString& raw)
{
    ParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty scale");
        return result;
    }

    bool percent = sanitized.endsWith(QLatin1Char('%'));
    if (percent)
        sanitized.chop(1);

    bool ok = false;
    double magnitude = sanitized.toDouble(&ok);
    if (!ok) {
        result.error = QObject::tr("invalid scale");
        return result;
    }

    if (percent)
        magnitude /= 100.0;

    if (magnitude <= 0.0) {
        result.error = QObject::tr("scale must be positive");
        return result;
    }

    result.ok = true;
    result.value = magnitude;
    result.display = raw.trimmed();
    if (percent && !result.display.endsWith(QLatin1Char('%')))
        result.display.append(QLatin1Char('%'));
    if (result.display.isEmpty())
        result.display = QObject::tr("%1x").arg(magnitude, 0, 'f', 3);
    return result;
}

ParseResult parseCount(const QString& raw)
{
    ParseResult result;
    QString sanitized = removeWhitespace(raw.trimmed());
    if (sanitized.isEmpty()) {
        result.error = QObject::tr("empty count");
        return result;
    }

    QString lower = sanitized.toLower();
    if (lower.endsWith(QStringLiteral("sides"))) {
        sanitized.chop(5);
    } else if (lower.endsWith(QStringLiteral("side"))) {
        sanitized.chop(4);
    } else if (sanitized.endsWith(QLatin1Char('s')) || sanitized.endsWith(QLatin1Char('S'))) {
        sanitized.chop(1);
    }

    if (sanitized.isEmpty()) {
        result.error = QObject::tr("invalid count");
        return result;
    }

    bool ok = false;
    const int count = sanitized.toInt(&ok);
    if (!ok) {
        result.error = QObject::tr("invalid count");
        return result;
    }

    if (count < 3) {
        result.error = QObject::tr("sides must be at least 3");
        return result;
    }

    result.ok = true;
    result.value = static_cast<double>(count);
    result.display = QObject::tr("%1 sides").arg(count);
    return result;
}

ParseResult parseMeasurement(const QString& raw, const QString& unitSystem, Tool::MeasurementKind kind)
{
    switch (kind) {
    case Tool::MeasurementKind::Distance:
        return parseDistance(raw, unitSystem);
    case Tool::MeasurementKind::Angle:
        return parseAngle(raw);
    case Tool::MeasurementKind::Count:
        return parseCount(raw);
    case Tool::MeasurementKind::Scale:
        return parseScale(raw);
    default:
        break;
    }

    ParseResult result;
    result.error = QObject::tr("unsupported measurement");
    return result;
}

QString measurementHintForKind(Tool::MeasurementKind kind)
{
    switch (kind) {
    case Tool::MeasurementKind::Distance:
        return QObject::tr("Length (e.g. 12'6\" or 3.5m)");
    case Tool::MeasurementKind::Angle:
        return QObject::tr("Angle (e.g. 45° or 0.79rad)");
    case Tool::MeasurementKind::Count:
        return QObject::tr("Sides (e.g. 6 or 12s)");
    case Tool::MeasurementKind::Scale:
        return QObject::tr("Scale (e.g. 2 or 150%)");
    default:
        return QObject::tr("Measurements");
    }
}

} // namespace Measurement

