#pragma once

#include <QString>
#include <QVector3D>
#include <optional>
#include <vector>

class CameraController;

class ViewPresetManager {
public:
    enum class StandardView {
        Iso,
        Top,
        Bottom,
        Front,
        Back,
        Left,
        Right
    };

    struct Preset {
        QString id;
        QString label;
        float yaw;
        float pitch;
        QVector3D target;
        float distance;
    };

    ViewPresetManager();

    const Preset* preset(StandardView view) const;
    const Preset* presetById(const QString& id) const;

    bool applyPreset(StandardView view, CameraController& camera) const;
    bool applyPreset(const QString& id, CameraController& camera) const;

    QString idFor(StandardView view) const;
    std::optional<StandardView> viewForId(const QString& id) const;
    QString labelForId(const QString& id) const;

private:
    struct Definition {
        StandardView view;
        Preset preset;
    };

    std::vector<Definition> presets;
};

