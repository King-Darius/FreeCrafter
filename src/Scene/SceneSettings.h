#pragma once

#include <iosfwd>

namespace Scene {

class SceneSettings {
public:
    SceneSettings();

    bool sectionPlanesVisible() const { return planesVisible; }
    void setSectionPlanesVisible(bool value) { planesVisible = value; }

    bool sectionFillsVisible() const { return fillsVisible; }
    void setSectionFillsVisible(bool value) { fillsVisible = value; }

    void reset();
    void serialize(std::ostream& os) const;
    bool deserialize(std::istream& is);

private:
    bool planesVisible = true;
    bool fillsVisible = true;
};

} // namespace Scene
