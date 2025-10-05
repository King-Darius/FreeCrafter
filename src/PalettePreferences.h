#pragma once

#include <QObject>
#include <QVector4D>
#include <QList>
#include <QString>

#include "Scene/SceneSettings.h"

namespace Scene {
class Document;
}

class PalettePreferences : public QObject {
    Q_OBJECT
public:
    struct PaletteInfo {
        QString id;
        QString label;
        QString description;
        QVector4D fill;
        QVector4D edge;
        QVector4D highlight;
    };

    struct ColorSet {
        QVector4D fill;
        QVector4D fillSelected;
        QVector4D edge;
        QVector4D edgeSelected;
        QVector4D curve;
        QVector4D curveSelected;
        QVector4D hiddenEdge;
        QVector4D hiddenEdgeSelected;
        QVector4D hiddenCurve;
        QVector4D hiddenCurveSelected;
        QVector4D highlight;
    };

    explicit PalettePreferences(QObject* parent = nullptr);

    QList<PaletteInfo> availablePalettes() const;
    QString activePaletteId() const;
    QString activePaletteLabel() const;
    ColorSet activeColors() const;

    bool setActivePalette(const QString& id);
    void attachDocument(Scene::Document* document);
    void refreshFromDocument();

    static ColorSet colorsFromState(const Scene::SceneSettings::PaletteState& state);
    static Scene::SceneSettings::PaletteState stateFromDefinition(const PaletteInfo& definition);

signals:
    void paletteChanged(const ColorSet& colors);

private:
    void applyState(const Scene::SceneSettings::PaletteState& state, bool emitSignal);
    const PaletteInfo* findPalette(const QString& id) const;
    void syncDocument();

    QList<PaletteInfo> paletteDefinitions;
    QString currentId;
    QString currentLabel;
    ColorSet currentColors;
    Scene::SceneSettings::PaletteState currentState;
    Scene::Document* attachedDocument = nullptr;
};

