#include "SceneSettings.h"

#include <istream>
#include <ostream>

namespace Scene {

SceneSettings::SceneSettings()
{
    reset();
}

void SceneSettings::reset()
{
    planesVisible = true;
    fillsVisible = true;
}

void SceneSettings::serialize(std::ostream& os) const
{
    os << (planesVisible ? 1 : 0) << ' ' << (fillsVisible ? 1 : 0) << '\n';
}

bool SceneSettings::deserialize(std::istream& is)
{
    int planes = 1;
    int fills = 1;
    if (!(is >> planes >> fills)) {
        return false;
    }
    planesVisible = planes != 0;
    fillsVisible = fills != 0;
    return true;
}

} // namespace Scene
