#pragma once

#include "SectionPlane.h"
#include "SceneSettings.h"
#include "../GeometryKernel/GeometryKernel.h"

#include <string>
#include <vector>

namespace Scene {

struct TextAnnotation {
    Vector3 position;
    std::string text;
};

struct LinearDimension {
    Vector3 start;
    Vector3 end;
    float value = 0.0f;
};

struct GuideLine {
    Vector3 start;
    Vector3 end;
    float length = 0.0f;
};

struct AngleGuide {
    Vector3 vertex;
    Vector3 legA;
    Vector3 legB;
    float angle = 0.0f;
};

struct AxesGuide {
    Vector3 origin;
    Vector3 xAxis;
    Vector3 yAxis;
    Vector3 zAxis;
};

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

    TextAnnotation& addTextAnnotation(const TextAnnotation& annotation);
    std::vector<TextAnnotation>& textAnnotations() { return annotations; }
    const std::vector<TextAnnotation>& textAnnotations() const { return annotations; }

    LinearDimension& addLinearDimension(const LinearDimension& dimension);
    std::vector<LinearDimension>& linearDimensions() { return dimensions; }
    const std::vector<LinearDimension>& linearDimensions() const { return dimensions; }

    GuideLine& addGuideLine(const GuideLine& guide);
    std::vector<GuideLine>& guideLines() { return guides; }
    const std::vector<GuideLine>& guideLines() const { return guides; }

    AngleGuide& addAngleGuide(const AngleGuide& guide);
    std::vector<AngleGuide>& angleGuides() { return angleMeasurements; }
    const std::vector<AngleGuide>& angleGuides() const { return angleMeasurements; }

    AxesGuide& addAxesGuide(const AxesGuide& guide);
    std::vector<AxesGuide>& axesGuides() { return axes; }
    const std::vector<AxesGuide>& axesGuides() const { return axes; }

private:
    GeometryKernel geometryKernel;
    std::vector<SectionPlane> planes;
    SceneSettings sceneSettings;
    std::vector<TextAnnotation> annotations;
    std::vector<LinearDimension> dimensions;
    std::vector<GuideLine> guides;
    std::vector<AngleGuide> angleMeasurements;
    std::vector<AxesGuide> axes;
};

} // namespace Scene
