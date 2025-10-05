#include "Document.h"

#include "SectionPlane.h"
#include "SceneSettings.h"

#include <fstream>
#include <string>

namespace Scene {

SectionPlane& Document::addSectionPlane(const SectionPlane& plane)
{
    planes.push_back(plane);
    return planes.back();
}

void Document::clearSectionPlanes()
{
    planes.clear();
}

void Document::reset()
{
    geometryKernel.clear();
    planes.clear();
    sceneSettings.reset();
}

bool Document::saveToFile(const std::string& filename) const
{
    std::ofstream os(filename, std::ios::out | std::ios::trunc);
    if (!os) {
        return false;
    }
    os << "FCM 3\n";
    os << "BEGIN_GEOMETRY\n";
    geometryKernel.saveToStream(os);
    os << "END_GEOMETRY\n";

    os << "BEGIN_SECTION_PLANES " << planes.size() << "\n";
    for (const auto& plane : planes) {
        plane.serialize(os);
    }
    os << "END_SECTION_PLANES\n";

    os << "BEGIN_SETTINGS\n";
    sceneSettings.serialize(os);
    os << "END_SETTINGS\n";
    return true;
}

bool Document::loadFromFile(const std::string& filename)
{
    std::ifstream is(filename);
    if (!is) {
        return false;
    }
    std::string tag;
    int version = 0;
    if (!(is >> tag >> version)) {
        return false;
    }
    if (tag != "FCM") {
        return false;
    }

    if (version <= 1) {
        bool loaded = geometryKernel.loadFromFile(filename);
        planes.clear();
        sceneSettings.reset();
        return loaded;
    }

    geometryKernel.clear();
    planes.clear();
    sceneSettings.reset();

    std::string token;
    while (is >> token) {
        if (token == "BEGIN_GEOMETRY") {
            geometryKernel.loadFromStream(is, "END_GEOMETRY");
        } else if (token == "BEGIN_SECTION_PLANES") {
            size_t count = 0;
            is >> count;
            planes.clear();
            planes.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                SectionPlane plane;
                if (!plane.deserialize(is)) {
                    break;
                }
                planes.push_back(plane);
            }
        } else if (token == "END_SECTION_PLANES") {
            continue;
        } else if (token == "BEGIN_SETTINGS") {
            sceneSettings.deserialize(is, version);
        } else if (token == "END_SETTINGS" || token == "END_GEOMETRY") {
            continue;
        }
    }
    return true;
}

} // namespace Scene
