#include "Navigation/ViewPresetManager.h"

#include "CameraController.h"

namespace {

constexpr float kDefaultDistance = 20.0f;

ViewPresetManager::Preset makePreset(const char* id, const char* label, float yaw, float pitch,
                                     const QVector3D& target = QVector3D(0.0f, 0.0f, 0.0f),
                                     float distance = kDefaultDistance)
{
    ViewPresetManager::Preset preset;
    preset.id = QString::fromUtf8(id);
    preset.label = QString::fromUtf8(label);
    preset.yaw = yaw;
    preset.pitch = pitch;
    preset.target = target;
    preset.distance = distance;
    return preset;
}

} // namespace

ViewPresetManager::ViewPresetManager()
{
    presets = {
        { StandardView::Iso, makePreset("iso", "Iso", 45.0f, -35.264f) },
        { StandardView::Top, makePreset("top", "Top", 0.0f, -90.0f) },
        { StandardView::Bottom, makePreset("bottom", "Bottom", 0.0f, 90.0f) },
        { StandardView::Front, makePreset("front", "Front", 0.0f, 0.0f) },
        { StandardView::Back, makePreset("back", "Back", 180.0f, 0.0f) },
        { StandardView::Left, makePreset("left", "Left", 270.0f, 0.0f) },
        { StandardView::Right, makePreset("right", "Right", 90.0f, 0.0f) },
    };
}

const ViewPresetManager::Preset* ViewPresetManager::preset(StandardView view) const
{
    for (const auto& definition : presets) {
        if (definition.view == view)
            return &definition.preset;
    }
    return nullptr;
}

const ViewPresetManager::Preset* ViewPresetManager::presetById(const QString& id) const
{
    for (const auto& definition : presets) {
        if (definition.preset.id.compare(id, Qt::CaseInsensitive) == 0)
            return &definition.preset;
    }
    return nullptr;
}

bool ViewPresetManager::applyPreset(StandardView view, CameraController& camera) const
{
    if (const auto* p = preset(view)) {
        camera.setYawPitch(p->yaw, p->pitch);
        camera.setTarget(p->target.x(), p->target.y(), p->target.z());
        camera.setDistance(p->distance);
        return true;
    }
    return false;
}

bool ViewPresetManager::applyPreset(const QString& id, CameraController& camera) const
{
    if (const auto* p = presetById(id)) {
        camera.setYawPitch(p->yaw, p->pitch);
        camera.setTarget(p->target.x(), p->target.y(), p->target.z());
        camera.setDistance(p->distance);
        return true;
    }
    return false;
}

QString ViewPresetManager::idFor(StandardView view) const
{
    if (const auto* p = preset(view))
        return p->id;
    return QString();
}

std::optional<ViewPresetManager::StandardView> ViewPresetManager::viewForId(const QString& id) const
{
    for (const auto& definition : presets) {
        if (definition.preset.id.compare(id, Qt::CaseInsensitive) == 0)
            return definition.view;
    }
    return std::nullopt;
}

QString ViewPresetManager::labelForId(const QString& id) const
{
    if (const auto* p = presetById(id))
        return p->label;
    return QString();
}

