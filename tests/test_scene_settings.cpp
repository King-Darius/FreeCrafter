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

    std::ostringstream out;
    settings.serialize(out);
    std::string serialized = out.str();
    assert(serialized.find("palette lavender") != std::string::npos);

    std::istringstream in(serialized + "END_SETTINGS\n");
    Scene::SceneSettings loaded;
    bool ok = loaded.deserialize(in, 3);
    assert(ok);
    const auto& palette = loaded.palette();
    assert(palette.id == "lavender");
    assert(std::fabs(palette.fill.r - custom.fill.r) < 1e-6f);
    assert(std::fabs(palette.edge.g - custom.edge.g) < 1e-6f);
    assert(std::fabs(palette.highlight.b - custom.highlight.b) < 1e-6f);

    std::istringstream legacy("1 0\n");
    Scene::SceneSettings legacySettings;
    bool legacyOk = legacySettings.deserialize(legacy, 2);
    assert(legacyOk);
    assert(legacySettings.sectionPlanesVisible());
    assert(!legacySettings.sectionFillsVisible());

    return 0;
}

