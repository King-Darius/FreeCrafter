#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

#include "../GeometryKernel/Vector3.h"

class GeometryKernel;

namespace Interaction {

enum class InferenceSnapType {
    None = 0,
    Endpoint,
    Midpoint,
    Intersection,
    FaceCenter,
    OnEdge,
    OnFace,
    Axis,
    Parallel,
    Perpendicular
};

struct PickRay {
    Vector3 origin;
    Vector3 direction;
};

struct InferenceResult {
    InferenceSnapType type = InferenceSnapType::None;
    Vector3 position;
    Vector3 direction;
    Vector3 reference;
    float distance = std::numeric_limits<float>::infinity();
    bool locked = false;

    bool isValid() const { return type != InferenceSnapType::None; }
};

struct InferenceContext {
    PickRay ray;
    float maxSnapDistance = 0.35f;
    float pixelRadius = 6.0f;
    Vector3 cameraTarget;
};

class InferenceEngine {
public:
    InferenceEngine();

    void ensureIndex(const GeometryKernel& geometry);
    InferenceResult query(const GeometryKernel& geometry, const InferenceContext& context);
    void invalidate();

private:
    std::size_t computeStamp(const GeometryKernel& geometry) const;
    void rebuild(const GeometryKernel& geometry);

    struct FaceFeature;
    struct EdgeFeature;

    std::vector<Vector3> endpointPositions;
    std::vector<Vector3> intersectionPositions;
    std::vector<Vector3> midpointPositions;
    std::vector<FaceFeature> faceFeatures;
    std::vector<EdgeFeature> edgeFeatures;
    std::vector<Vector3> faceCenterPositions;

    class SimpleKdTree;
    std::unique_ptr<SimpleKdTree> endpointTree;
    std::unique_ptr<SimpleKdTree> intersectionTree;
    std::unique_ptr<SimpleKdTree> midpointTree;
    std::unique_ptr<SimpleKdTree> faceCenterTree;

    std::size_t cachedStamp;
    bool dirty;
};

const char* toString(InferenceSnapType type);

} // namespace Interaction
