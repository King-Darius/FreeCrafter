#pragma once
#include <vector>
#include "Vector3.h"

namespace MeshUtils {
bool nearlyEqual(const Vector3& a, const Vector3& b, float epsilon = 1e-5f);
float distanceSquared(const Vector3& a, const Vector3& b);
std::vector<Vector3> weldSequential(const std::vector<Vector3>& pts, float epsilon = 1e-5f);
std::vector<Vector3> collapseTinyEdges(const std::vector<Vector3>& pts, float minEdge = 1e-4f);
Vector3 computePolygonNormal(const std::vector<Vector3>& polygon);
}
