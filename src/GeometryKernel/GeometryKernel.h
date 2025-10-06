#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "GeometryObject.h"
#include "Curve.h"
#include "Solid.h"

class GeometryKernel {
public:
    GeometryKernel() = default;
    GeometryObject* addCurve(const std::vector<Vector3>& points);
    GeometryObject* extrudeCurve(GeometryObject* curveObj, float height);
    GeometryObject* addObject(std::unique_ptr<GeometryObject> object);
    GeometryObject* cloneObject(const GeometryObject& source);
    void deleteObject(GeometryObject* obj);
    void clear();
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    void saveToStream(std::ostream& os) const;
    void loadFromStream(std::istream& is, const std::string& terminator);
    const std::vector<std::unique_ptr<GeometryObject>>& getObjects() const { return objects; }

    void assignMaterial(const GeometryObject* object, const std::string& materialName);
    std::string getMaterial(const GeometryObject* object) const;
    const std::unordered_map<const GeometryObject*, std::string>& getMaterials() const { return materialAssignments; }

    struct TextAnnotation {
        Vector3 position;
        std::string text;
        float height = 1.0f;
    };

    void addTextAnnotation(const Vector3& position, std::string text, float height);
    const std::vector<TextAnnotation>& getTextAnnotations() const { return textAnnotations; }

    struct LinearDimension {
        Vector3 start;
        Vector3 end;
        float value = 0.0f;
    };

    void addDimension(const Vector3& start, const Vector3& end);
    const std::vector<LinearDimension>& getDimensions() const { return dimensions; }

    struct GuideState {
        struct GuideLine {
            Vector3 start;
            Vector3 end;
        };
        struct GuidePoint {
            Vector3 position;
        };
        struct GuideAngle {
            Vector3 origin;
            Vector3 startDirection;
            Vector3 endDirection;
            float angleDegrees = 0.0f;
        };

        std::vector<GuideLine> lines;
        std::vector<GuidePoint> points;
        std::vector<GuideAngle> angles;
    };

    void addGuideLine(const Vector3& start, const Vector3& end);
    void addGuidePoint(const Vector3& position);
    void addGuideAngle(const Vector3& origin, const Vector3& startDirection, const Vector3& endDirection);
    void clearGuides();
    const GuideState& getGuides() const { return guides; }

    struct AxesState {
        Vector3 origin{ 0.0f, 0.0f, 0.0f };
        Vector3 xAxis{ 1.0f, 0.0f, 0.0f };
        Vector3 yAxis{ 0.0f, 1.0f, 0.0f };
        Vector3 zAxis{ 0.0f, 0.0f, 1.0f };
        bool valid = true;
    };

    void setAxes(const Vector3& origin, const Vector3& xDirection, const Vector3& yDirection);
    void resetAxes();
    const AxesState& getAxesState() const { return axes; }

    struct MeshBuffer {
        std::vector<Vector3> positions;
        std::vector<Vector3> normals;
        std::vector<std::uint32_t> indices;
    };

    MeshBuffer buildMeshBuffer(const GeometryObject& object) const;
    static std::array<float, 16> identityTransform();
    static HalfEdgeMesh meshFromIndexedData(const std::vector<Vector3>& positions,
                                           const std::vector<std::uint32_t>& indices);

private:
    std::vector<std::unique_ptr<GeometryObject>> objects;
    std::unordered_map<const GeometryObject*, std::string> materialAssignments;
    std::vector<TextAnnotation> textAnnotations;
    std::vector<LinearDimension> dimensions;
    GuideState guides;
    AxesState axes;
};
