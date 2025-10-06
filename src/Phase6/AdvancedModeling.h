#pragma once

#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"
#include "../GeometryKernel/Vector3.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Phase6 {

enum class CornerStyle {
    Fillet,
    Chamfer
};

struct CornerOverride {
    std::size_t cornerIndex = 0;
    float radius = -1.0f;
    int segments = -1;
    CornerStyle style = CornerStyle::Fillet;
    std::optional<bool> hardEdge;
};

struct RoundCornerOptions {
    float radius = 0.1f;
    int segments = 6;
    CornerStyle style = CornerStyle::Fillet;
    bool tagHardEdges = false;
    std::vector<CornerOverride> overrides;
};

class RoundCorner {
public:
    explicit RoundCorner(GeometryKernel& kernel);
    bool filletCurve(Curve& curve, const RoundCornerOptions& options) const;
    std::unique_ptr<Curve> createFilleted(const Curve& curve, const RoundCornerOptions& options) const;

private:
    GeometryKernel& geometry;
};

struct LoftOptions {
    int sections = 8;
    bool closeRails = false;
    bool smoothNormals = true;
    float twistDegrees = 0.0f;
    int smoothingPasses = 1;
    bool symmetricPairing = false;
    std::vector<Vector3> railA;
    std::vector<Vector3> railB;
};

class CurveIt {
public:
    explicit CurveIt(GeometryKernel& kernel);
    Solid* loft(const Curve& start, const Curve& end, const LoftOptions& options) const;

private:
    GeometryKernel& geometry;
};

struct PushPullOptions {
    float distance = 0.1f;
    bool createCaps = true;
    bool softenNormals = true;
    bool generateInnerShell = true;
    float maxThickness = 10.0f;
};

class PushAndPull {
public:
    bool thicken(Solid& solid, const PushPullOptions& options) const;
};

struct SurfaceDrawOptions {
    float samplingDistance = 0.05f;
    float projectionOffset = 0.0f;
    bool remesh = true;
};

class Surface {
public:
    explicit Surface(GeometryKernel& kernel);
    Curve* drawPolylineOnSolid(const Solid& solid, const std::vector<Vector3>& path, const SurfaceDrawOptions& options) const;

private:
    GeometryKernel& geometry;
};

struct KnifeOptions {
    int samplesPerSegment = 16;
    float extrusionHeight = 0.01f;
    bool removeInterior = true;
    float cutWidth = 0.001f;
};

class BezierKnife {
public:
    explicit BezierKnife(GeometryKernel& kernel);
    Curve* cut(const Solid& solid, const std::vector<Vector3>& controlPoints, const KnifeOptions& options) const;

private:
    GeometryKernel& geometry;
};

struct QuadConversionOptions {
    float mergeThreshold = 1e-4f;
    bool validateTopology = true;
    bool fillHoles = true;
};

class QuadTools {
public:
    bool retopologizeToQuads(Solid& solid, const QuadConversionOptions& options) const;
};

struct SubdivisionOptions {
    int levels = 1;
    bool preserveCreases = true;
    bool generateCage = true;
};

class SubD {
public:
    bool subdivide(Solid& solid, const SubdivisionOptions& options) const;
};

struct WeldOptions {
    float tolerance = 1e-4f;
    Vector3 direction{ 0.0f, 1.0f, 0.0f };
    float directionalWeight = 0.0f;
};

class Weld {
public:
    bool apply(Solid& solid, const WeldOptions& options) const;
    bool apply(Curve& curve, const WeldOptions& options) const;
};

struct SoftSelectionOptions {
    float radius = 1.0f;
    float falloff = 2.0f;
    Vector3 translation{ 0.0f, 0.0f, 0.0f };
    Vector3 rotationAxis{ 0.0f, 1.0f, 0.0f };
    float rotationDegrees = 0.0f;
    Vector3 scaling{ 1.0f, 1.0f, 1.0f };
};

class VertexTools {
public:
    bool applySoftTranslation(Solid& solid, const std::vector<int>& seedVertices, const SoftSelectionOptions& options) const;
};

struct CleanOptions {
    float minEdgeLength = 1e-4f;
    float minFaceArea = 1e-6f;
    bool removeUnusedVertices = true;
    bool mergeCoplanarFaces = true;
};

class Clean {
public:
    bool apply(Solid& solid, const CleanOptions& options) const;
    bool apply(Curve& curve, const CleanOptions& options) const;
};

struct ClothOptions {
    float stiffness = 0.9f;
    float damping = 0.02f;
    int solverIterations = 5;
    float timestep = 1.0f / 60.0f;
    Vector3 gravity{ 0.0f, -9.8f, 0.0f };
    std::vector<int> pinnedVertices;
    std::vector<float> weightMap;
    std::vector<const Solid*> colliders;
};

class ClothEngine {
public:
    void simulate(Solid& clothSurface, const ClothOptions& options, int frames) const;
};

struct RevolveOptions {
    Vector3 axisPoint{ 0.0f, 0.0f, 0.0f };
    Vector3 axisDirection{ 0.0f, 1.0f, 0.0f };
    float angleDegrees = 360.0f;
    int segments = 16;
};

struct SweepOptions {
    int samples = 32;
    bool capEnds = true;
};

struct ShellOptions {
    float thickness = 0.05f;
    bool capHoles = true;
};

struct PatternOptions {
    int count = 2;
    Vector3 translationStep{ 0.0f, 0.0f, 0.0f };
    float rotationStepDegrees = 0.0f;
};

struct SplitOptions {
    Vector3 planePoint{ 0.0f, 0.0f, 0.0f };
    Vector3 planeNormal{ 0.0f, 1.0f, 0.0f };
    bool keepPositive = true;
};

class CADDesigner {
public:
    explicit CADDesigner(GeometryKernel& kernel);
    Solid* revolve(const Curve& profile, const RevolveOptions& options) const;
    Solid* sweep(const Curve& profile, const std::vector<Vector3>& path, const SweepOptions& options) const;
    Solid* mirror(const Solid& solid, const Vector3& planePoint, const Vector3& planeNormal) const;
    Solid* shell(const Solid& solid, const ShellOptions& options) const;
    std::vector<Solid*> pattern(const Solid& solid, const PatternOptions& options) const;
    Solid* split(const Solid& solid, const SplitOptions& options) const;
    Curve* imprint(const Solid& solid, const std::vector<Vector3>& path) const;

private:
    GeometryKernel& geometry;
};

} // namespace Phase6

