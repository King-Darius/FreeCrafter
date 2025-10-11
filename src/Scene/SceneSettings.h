#pragma once

#include <iosfwd>
#include <string>

#include <cstddef>

namespace Scene {

class SceneSettings {
public:
    SceneSettings();

    bool sectionPlanesVisible() const { return planesVisible; }
    void setSectionPlanesVisible(bool value) { planesVisible = value; }

    bool sectionFillsVisible() const { return fillsVisible; }
    void setSectionFillsVisible(bool value) { fillsVisible = value; }

    bool guidesVisible() const { return guidesVisibleFlag; }
    void setGuidesVisible(bool value) { guidesVisibleFlag = value; }

    struct GridSettings {
        float majorSpacing = 1.0f;
        int minorDivisions = 4;
        int majorExtent = 50;
    };

    const GridSettings& grid() const { return gridSettings; }
    void setGrid(const GridSettings& grid) { gridSettings = grid; }

    enum class ShadowQuality {
        Low = 0,
        Medium = 1,
        High = 2
    };

    struct ShadowSettings {
        bool enabled = false;
        ShadowQuality quality = ShadowQuality::Medium;
        float strength = 0.65f;
        float bias = 0.0035f;
    };

    const ShadowSettings& shadows() const { return shadowSettings; }
    void setShadows(const ShadowSettings& settings) { shadowSettings = settings; }

    struct Color {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct PaletteState {
        std::string id;
        Color fill;
        Color edge;
        Color highlight;
    };

    const PaletteState& palette() const { return paletteState; }
    void setPalette(const PaletteState& state) { paletteState = state; }

    void reset();
    void serialize(std::ostream& os) const;
    bool deserialize(std::istream& is, int version);

private:
    bool planesVisible = true;
    bool fillsVisible = true;
    bool guidesVisibleFlag = true;
    GridSettings gridSettings;
    ShadowSettings shadowSettings;
    PaletteState paletteState;
};

} // namespace Scene
