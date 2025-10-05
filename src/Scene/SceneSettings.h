#pragma once

#include <iosfwd>
#include <string>

namespace Scene {

class SceneSettings {
public:
    SceneSettings();

    bool sectionPlanesVisible() const { return planesVisible; }
    void setSectionPlanesVisible(bool value) { planesVisible = value; }

    bool sectionFillsVisible() const { return fillsVisible; }
    void setSectionFillsVisible(bool value) { fillsVisible = value; }

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
    PaletteState paletteState;
};

} // namespace Scene
