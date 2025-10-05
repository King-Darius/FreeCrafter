#include "PalettePreferences.h"

#include <algorithm>


#include "Scene/Document.h"

namespace {

QVector4D mix(const QVector4D& a, const QVector4D& b, float t)
{
    return a * (1.0f - t) + b * t;
}

QVector4D applyAlpha(const QVector4D& color, float alpha)
{
    QVector4D result = color;
    result.setW(alpha);
    return result;
}

QVector4D clampToUnit(const QVector4D& color)
{
    return QVector4D(std::clamp(color.x(), 0.0f, 1.0f),
                     std::clamp(color.y(), 0.0f, 1.0f),
                     std::clamp(color.z(), 0.0f, 1.0f),
                     std::clamp(color.w(), 0.0f, 1.0f));
}

Scene::SceneSettings::Color toSceneColor(const QVector4D& color)
{
    Scene::SceneSettings::Color c;
    c.r = color.x();
    c.g = color.y();
    c.b = color.z();
    c.a = color.w();
    return c;
}

QVector4D toQtColor(const Scene::SceneSettings::Color& color)
{
    return QVector4D(color.r, color.g, color.b, color.a);
}

} // namespace

PalettePreferences::PalettePreferences(QObject* parent)
    : QObject(parent)
{
    paletteDefinitions = {
        {
            QStringLiteral("softGreen"),
            tr("Soft Green"),
            tr("Muted sage fill with pine accents and teal highlights."),
            QVector4D(0.74f, 0.83f, 0.78f, 1.0f),
            QVector4D(0.20f, 0.30f, 0.24f, 1.0f),
            QVector4D(0.36f, 0.62f, 0.49f, 1.0f)
        },
        {
            QStringLiteral("lavender"),
            tr("Lavender"),
            tr("Dusty violet surfaces with plum outlines and amethyst selection."),
            QVector4D(0.82f, 0.78f, 0.90f, 1.0f),
            QVector4D(0.30f, 0.24f, 0.44f, 1.0f),
            QVector4D(0.60f, 0.44f, 0.80f, 1.0f)
        },
        {
            QStringLiteral("powderBlue"),
            tr("Powder Blue"),
            tr("Sky-tinted fills with deep slate edges and cobalt highlights."),
            QVector4D(0.78f, 0.84f, 0.92f, 1.0f),
            QVector4D(0.22f, 0.28f, 0.42f, 1.0f),
            QVector4D(0.36f, 0.58f, 0.86f, 1.0f)
        }
    };

    if (!paletteDefinitions.isEmpty()) {
        applyState(stateFromDefinition(paletteDefinitions.front()), false);
    }
}

QList<PalettePreferences::PaletteInfo> PalettePreferences::availablePalettes() const
{
    return paletteDefinitions;
}

QString PalettePreferences::activePaletteId() const
{
    return currentId;
}

QString PalettePreferences::activePaletteLabel() const
{
    return currentLabel.isEmpty() ? currentId : currentLabel;
}

PalettePreferences::ColorSet PalettePreferences::activeColors() const
{
    return currentColors;
}

bool PalettePreferences::setActivePalette(const QString& id)
{
    const PaletteInfo* info = findPalette(id);
    if (!info)
        return false;

    Scene::SceneSettings::PaletteState state = stateFromDefinition(*info);
    applyState(state, true);
    syncDocument();
    return true;
}

void PalettePreferences::attachDocument(Scene::Document* document)
{
    attachedDocument = document;
    if (attachedDocument) {
        applyState(attachedDocument->settings().palette(), true);
    }
}

void PalettePreferences::refreshFromDocument()
{
    if (!attachedDocument)
        return;
    applyState(attachedDocument->settings().palette(), true);
}

PalettePreferences::ColorSet PalettePreferences::colorsFromState(const Scene::SceneSettings::PaletteState& state)
{
    QVector4D fill = toQtColor(state.fill);
    QVector4D edge = toQtColor(state.edge);
    QVector4D highlight = toQtColor(state.highlight);

    ColorSet set;
    set.fill = clampToUnit(fill);
    set.fillSelected = clampToUnit(mix(fill, highlight, 0.35f));
    set.edge = clampToUnit(edge);
    set.edgeSelected = clampToUnit(mix(edge, highlight, 0.55f));
    set.curve = set.edge;
    set.curveSelected = clampToUnit(mix(edge, highlight, 0.7f));
    QVector4D neutral(0.5f, 0.5f, 0.5f, 1.0f);
    set.hiddenEdge = clampToUnit(applyAlpha(mix(edge, neutral, 0.6f), 0.65f));
    set.hiddenEdgeSelected = clampToUnit(applyAlpha(mix(set.edgeSelected, neutral, 0.4f), 0.75f));
    set.hiddenCurve = set.hiddenEdge;
    set.hiddenCurveSelected = set.hiddenEdgeSelected;
    set.highlight = clampToUnit(highlight);
    return set;
}

Scene::SceneSettings::PaletteState PalettePreferences::stateFromDefinition(const PaletteInfo& definition)
{
    Scene::SceneSettings::PaletteState state;
    state.id = definition.id.toStdString();
    state.fill = toSceneColor(definition.fill);
    state.edge = toSceneColor(definition.edge);
    state.highlight = toSceneColor(definition.highlight);
    return state;
}

void PalettePreferences::applyState(const Scene::SceneSettings::PaletteState& state, bool emitSignal)
{
    Scene::SceneSettings::PaletteState clamped = state;
    currentState = clamped;
    currentColors = colorsFromState(clamped);
    currentId = QString::fromStdString(clamped.id);
    if (const PaletteInfo* info = findPalette(currentId)) {
        currentLabel = info->label;
    } else {
        currentLabel = QString();
    }
    if (emitSignal)
        emit paletteChanged(currentColors);
}

const PalettePreferences::PaletteInfo* PalettePreferences::findPalette(const QString& id) const
{
    for (const auto& info : paletteDefinitions) {
        if (info.id == id)
            return &info;
    }
    return nullptr;
}

void PalettePreferences::syncDocument()
{
    if (!attachedDocument)
        return;
    attachedDocument->settings().setPalette(currentState);
}

