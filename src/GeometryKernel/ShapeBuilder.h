#pragma once

#include "Vector3.h"

#include <vector>

namespace ShapeBuilder {

struct ArcDefinition {
    Vector3 center{ 0.0f, 0.0f, 0.0f };
    float radius = 0.0f;
    float startAngle = 0.0f;
    float endAngle = 0.0f;
    bool counterClockwise = true;
    int segments = 0;
};

struct BezierDefinition {
    Vector3 p0{ 0.0f, 0.0f, 0.0f };
    Vector3 h0{ 0.0f, 0.0f, 0.0f };
    Vector3 h1{ 0.0f, 0.0f, 0.0f };
    Vector3 p1{ 0.0f, 0.0f, 0.0f };
    int segments = 24;
};

std::vector<Vector3> buildCircle(const Vector3& center, const Vector3& radiusPoint, int segments);
std::vector<Vector3> buildRegularPolygon(const Vector3& center, const Vector3& radiusPoint, int sides);
std::vector<Vector3> buildArc(const ArcDefinition& definition);
bool solveArcThroughPoints(const Vector3& start, const Vector3& mid, const Vector3& end, ArcDefinition& outDefinition);
ArcDefinition makeArcFromCenter(const Vector3& center, float radius, float startAngle, float endAngle, bool counterClockwise, int segmentsHint);
std::vector<Vector3> buildBezier(const BezierDefinition& definition);

float angleBetween(const Vector3& center, const Vector3& point);

} // namespace ShapeBuilder

