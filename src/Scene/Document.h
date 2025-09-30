#pragma once

#include "SectionPlane.h"
#include "SceneSettings.h"
#include "../GeometryKernel/GeometryKernel.h"

#include <string>
#include <vector>

namespace Scene {

class Document {
public:
    Document() = default;

    GeometryKernel& geometry() { return geometryKernel; }
    const GeometryKernel& geometry() const { return geometryKernel; }

    std::vector<SectionPlane>& sectionPlanes() { return planes; }
    const std::vector<SectionPlane>& sectionPlanes() const { return planes; }

    SceneSettings& settings() { return sceneSettings; }
    const SceneSettings& settings() const { return sceneSettings; }

    SectionPlane& addSectionPlane(const SectionPlane& plane);
    void clearSectionPlanes();
    void reset();

    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

private:
    GeometryKernel geometryKernel;
    std::vector<SectionPlane> planes;
    SceneSettings sceneSettings;
};

} // namespace Scene
