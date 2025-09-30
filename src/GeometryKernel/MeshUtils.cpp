#include "MeshUtils.h"
#include <algorithm>

namespace MeshUtils {

bool nearlyEqual(const Vector3& a, const Vector3& b, float epsilon) {
    return distanceSquared(a, b) <= epsilon * epsilon;
}

float distanceSquared(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

std::vector<Vector3> weldSequential(const std::vector<Vector3>& pts, float epsilon) {
    std::vector<Vector3> result;
    result.reserve(pts.size());
    for (const auto& p : pts) {
        Vector3 projected(p.x, 0.0f, p.z);
        if (result.empty() || !nearlyEqual(projected, result.back(), epsilon)) {
            result.push_back(projected);
        }
    }
    if (result.size() > 1 && nearlyEqual(result.front(), result.back(), epsilon)) {
        result.pop_back();
    }
    return result;
}

std::vector<Vector3> collapseTinyEdges(const std::vector<Vector3>& pts, float minEdge) {
    if (pts.size() < 3) return pts;
    std::vector<Vector3> working = pts;
    bool changed = true;
    float minEdgeSq = minEdge * minEdge;
    while (changed && working.size() >= 3) {
        changed = false;
        for (size_t i = 0; i < working.size(); ++i) {
            size_t j = (i + 1) % working.size();
            if (distanceSquared(working[i], working[j]) < minEdgeSq) {
                working.erase(working.begin() + static_cast<long long>(j));
                changed = true;
                break;
            }
        }
    }
    if (working.size() > 1 && nearlyEqual(working.front(), working.back(), minEdge)) {
        working.pop_back();
    }
    return working;
}

Vector3 computePolygonNormal(const std::vector<Vector3>& polygon) {
    Vector3 normal(0.0f, 0.0f, 0.0f);
    if (polygon.size() < 3) return normal;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const Vector3& current = polygon[i];
        const Vector3& next = polygon[(i + 1) % polygon.size()];
        normal.x += (current.y - next.y) * (current.z + next.z);
        normal.y += (current.z - next.z) * (current.x + next.x);
        normal.z += (current.x - next.x) * (current.y + next.y);
    }
    return normal.normalized();
}

}
