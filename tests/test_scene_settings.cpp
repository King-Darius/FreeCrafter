#include <cassert>
#include <cmath>
#include <sstream>

#include "Scene/SceneSettings.h"

int main()
{
    Scene::SceneSettings settings;
    settings.reset();

    Scene::SceneSettings::PaletteState custom;
    custom.id = "lavender";
    custom.fill = {0.82f, 0.78f, 0.90f, 1.0f};
    custom.edge = {0.30f, 0.24f, 0.44f, 1.0f};
    custom.highlight = {0.60f, 0.44f, 0.80f, 1.0f};
    settings.setPalette(custom);

    Scene::SceneSettings::GridSettings grid;
    grid.majorSpacing = 2.5f;
    grid.minorDivisions = 8;
    grid.majorExtent = 60;
    settings.setGrid(grid);

    Scene::SceneSettings::ShadowSettings shadows;
    shadows.enabled = true;
    shadows.quality = Scene::SceneSettings::ShadowQuality::High;
    shadows.strength = 0.4f;
    shadows.bias = 0.0012f;
    settings.setShadows(shadows);
    settings.setGuidesVisible(false);

    std::ostringstream out;
    settings.serialize(out);
    std::string serialized = out.str();
    assert(serialized.find("palette lavender") != std::string::npos);
    assert(serialized.find("gridMajorSpacing 2.5") != std::string::npos);
    assert(serialized.find("shadowEnabled 1") != std::string::npos);

    std::istringstream in(serialized + "END_SETTINGS\n");
    Scene::SceneSettings loaded;
    bool ok = loaded.deserialize(in, 3);
    assert(ok);
    const auto& palette = loaded.palette();
    assert(palette.id == "lavender");
    assert(std::fabs(palette.fill.r - custom.fill.r) < 1e-6f);
    assert(std::fabs(palette.edge.g - custom.edge.g) < 1e-6f);
    assert(std::fabs(palette.highlight.b - custom.highlight.b) < 1e-6f);
    const auto& loadedGrid = loaded.grid();
    assert(std::fabs(loadedGrid.majorSpacing - grid.majorSpacing) < 1e-6f);
    assert(loadedGrid.minorDivisions == grid.minorDivisions);
    assert(loadedGrid.majorExtent == grid.majorExtent);
    const auto& loadedShadows = loaded.shadows();
    assert(loadedShadows.enabled);
    assert(loadedShadows.quality == Scene::SceneSettings::ShadowQuality::High);
    assert(std::fabs(loadedShadows.strength - shadows.strength) < 1e-6f);
    assert(std::fabs(loadedShadows.bias - shadows.bias) < 1e-6f);
    assert(!loaded.guidesVisible());

    Scene::SceneSettings::PaletteState outOfRange = custom;
    outOfRange.id = "wild";
    outOfRange.fill = {1.5f, -0.25f, 0.5f, 2.0f};
    outOfRange.edge = {-0.5f, 1.2f, 1.1f, -1.0f};
    outOfRange.highlight = {0.4f, 0.6f, 5.0f, 0.0f};
    settings.setPalette(outOfRange);
    const auto& sanitized = settings.palette();
    assert(sanitized.id == outOfRange.id);
    assert(std::fabs(sanitized.fill.r - 1.0f) < 1e-6f);
    assert(std::fabs(sanitized.fill.g - 0.0f) < 1e-6f);
    assert(std::fabs(sanitized.fill.a - 1.0f) < 1e-6f);
    assert(std::fabs(sanitized.edge.r - 0.0f) < 1e-6f);
    assert(std::fabs(sanitized.edge.g - 1.0f) < 1e-6f);
    assert(std::fabs(sanitized.edge.b - 1.0f) < 1e-6f);
    assert(std::fabs(sanitized.edge.a - 0.0f) < 1e-6f);
    assert(std::fabs(sanitized.highlight.r - 0.4f) < 1e-6f);
    assert(std::fabs(sanitized.highlight.g - 0.6f) < 1e-6f);
    assert(std::fabs(sanitized.highlight.b - 1.0f) < 1e-6f);
    assert(std::fabs(sanitized.highlight.a - 0.0f) < 1e-6f);

    std::istringstream legacy("1 0\n");
    Scene::SceneSettings legacySettings;
    bool legacyOk = legacySettings.deserialize(legacy, 2);
    assert(legacyOk);
    assert(legacySettings.sectionPlanesVisible());
    assert(!legacySettings.sectionFillsVisible());

    return 0;
}

