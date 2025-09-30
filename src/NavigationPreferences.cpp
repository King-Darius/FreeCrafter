#include "NavigationPreferences.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {
constexpr auto kConfigFileName = "navigation.json";
constexpr auto kDefaultResource = ":/config/navigation_default.json";

Qt::MouseButton parseButton(const QString& value)
{
    const QString lower = value.trimmed().toLower();
    if (lower == QLatin1String("left"))
        return Qt::LeftButton;
    if (lower == QLatin1String("right"))
        return Qt::RightButton;
    if (lower == QLatin1String("middle"))
        return Qt::MiddleButton;
    if (lower == QLatin1String("aux1"))
        return Qt::ExtraButton1;
    if (lower == QLatin1String("aux2"))
        return Qt::ExtraButton2;
    return Qt::NoButton;
}

Qt::KeyboardModifiers parseModifiers(const QJsonValue& value)
{
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    auto applyToken = [&](const QString& token) {
        const QString lower = token.trimmed().toLower();
        if (lower == QLatin1String("shift"))
            modifiers |= Qt::ShiftModifier;
        else if (lower == QLatin1String("ctrl") || lower == QLatin1String("control") || lower == QLatin1String("cmd")
                 || lower == QLatin1String("command"))
            modifiers |= Qt::ControlModifier;
        else if (lower == QLatin1String("alt"))
            modifiers |= Qt::AltModifier;
        else if (lower == QLatin1String("meta"))
            modifiers |= Qt::ControlModifier;
    };

    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const auto& item : array) {
            if (item.isString())
                applyToken(item.toString());
        }
    } else if (value.isString()) {
        applyToken(value.toString());
    }
    return modifiers;
}

} // namespace

NavigationPreferences::NavigationPreferences(QObject* parent)
    : QObject(parent)
{
    ensureConfigPath();
    loadDefaults();
    load();
    if (activeSchemeId.isEmpty() && !defaultSchemeId.isEmpty()) {
        setActiveScheme(defaultSchemeId);
    }
}

QList<NavigationPreferences::SchemeInfo> NavigationPreferences::availableSchemes() const
{
    QList<SchemeInfo> list;
    for (const auto& scheme : schemes) {
        list.append(scheme.info);
    }
    return list;
}

NavigationConfig NavigationPreferences::schemeConfig(const QString& id) const
{
    if (const SchemeDefinition* scheme = findScheme(id))
        return scheme->config;
    return NavigationConfig{};
}

bool NavigationPreferences::setActiveScheme(const QString& id)
{
    const QString target = id.isEmpty() ? defaultSchemeId : id;
    const SchemeDefinition* scheme = findScheme(target);
    if (!scheme)
        return false;

    bool changed = (activeSchemeId != scheme->info.id) || (activeConfig.zoomToCursor != scheme->config.zoomToCursor)
        || (activeConfig.invertWheel != scheme->config.invertWheel) || (activeConfig.wheelStep != scheme->config.wheelStep)
        || (activeConfig.dragBindings != scheme->config.dragBindings) || (activeConfig.wheelToolName != scheme->config.wheelToolName);

    activeSchemeId = scheme->info.id;
    activeConfig = scheme->config;

    if (changed)
        emit configChanged();
    return true;
}

void NavigationPreferences::setZoomToCursor(bool enabled)
{
    if (activeConfig.zoomToCursor == enabled)
        return;
    activeConfig.zoomToCursor = enabled;
    emit configChanged();
}

void NavigationPreferences::setInvertWheel(bool enabled)
{
    if (activeConfig.invertWheel == enabled)
        return;
    activeConfig.invertWheel = enabled;
    emit configChanged();
}

void NavigationPreferences::resetToDefaults()
{
    if (defaultSchemeId.isEmpty())
        return;
    setActiveScheme(defaultSchemeId);
}

void NavigationPreferences::save() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("version"), schemaVersion);
    obj.insert(QStringLiteral("activeScheme"), activeSchemeId);
    obj.insert(QStringLiteral("zoomToCursor"), activeConfig.zoomToCursor);
    obj.insert(QStringLiteral("invertWheel"), activeConfig.invertWheel);

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(obj).toJson());
}

void NavigationPreferences::ensureConfigPath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(base);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));
    configPath = dir.filePath(QString::fromLatin1(kConfigFileName));
}

void NavigationPreferences::loadDefaults()
{
    schemes.clear();
    QFile file(QString::fromLatin1(kDefaultResource));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject obj = doc.object();
    schemaVersion = obj.value(QStringLiteral("version")).toInt(schemaVersion);
    defaultSchemeId = obj.value(QStringLiteral("defaultScheme")).toString();

    const QJsonArray schemeArray = obj.value(QStringLiteral("schemes")).toArray();
    for (const auto& entry : schemeArray) {
        if (!entry.isObject())
            continue;
        const QJsonObject schemeObj = entry.toObject();
        SchemeDefinition def;
        def.info.id = schemeObj.value(QStringLiteral("id")).toString();
        if (def.info.id.isEmpty())
            continue;
        def.info.label = schemeObj.value(QStringLiteral("label")).toString(def.info.id);
        def.info.description = schemeObj.value(QStringLiteral("description")).toString();
        def.config.wheelToolName = schemeObj.value(QStringLiteral("wheelTool")).toString(QStringLiteral("ZoomTool")).toStdString();
        def.config.zoomToCursor = schemeObj.value(QStringLiteral("zoomToCursor")).toBool(true);
        def.config.invertWheel = schemeObj.value(QStringLiteral("invertWheel")).toBool(false);
        def.config.wheelStep = static_cast<float>(schemeObj.value(QStringLiteral("wheelStep")).toDouble(1.0));

        const QJsonArray bindings = schemeObj.value(QStringLiteral("dragBindings")).toArray();
        for (const auto& bindingValue : bindings) {
            if (!bindingValue.isObject())
                continue;
            const QJsonObject bindingObj = bindingValue.toObject();
            NavigationConfig::DragBinding binding;
            binding.button = parseButton(bindingObj.value(QStringLiteral("button")).toString());
            binding.modifiers = parseModifiers(bindingObj.value(QStringLiteral("modifiers")));
            binding.toolName = bindingObj.value(QStringLiteral("tool")).toString().toStdString();
            binding.temporary = bindingObj.value(QStringLiteral("temporary")).toBool(true);
            if (binding.button == Qt::NoButton || binding.toolName.empty())
                continue;
            def.config.dragBindings.push_back(binding);
        }

        schemes.push_back(def);
    }

    if (defaultSchemeId.isEmpty() && !schemes.isEmpty())
        defaultSchemeId = schemes.front().info.id;
}

void NavigationPreferences::load()
{
    QFile file(configPath);
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject obj = doc.object();
    const int version = obj.value(QStringLiteral("version")).toInt(schemaVersion);
    if (version != schemaVersion)
        return;

    const QString schemeId = obj.value(QStringLiteral("activeScheme")).toString();
    if (!schemeId.isEmpty())
        setActiveScheme(schemeId);

    activeConfig.zoomToCursor = obj.value(QStringLiteral("zoomToCursor")).toBool(activeConfig.zoomToCursor);
    activeConfig.invertWheel = obj.value(QStringLiteral("invertWheel")).toBool(activeConfig.invertWheel);

    emit configChanged();
}

const NavigationPreferences::SchemeDefinition* NavigationPreferences::findScheme(const QString& id) const
{
    for (const auto& scheme : schemes) {
        if (scheme.info.id == id)
            return &scheme;
    }
    return nullptr;
}
