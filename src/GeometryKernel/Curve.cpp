#include "Curve.h"

Curve::Curve(const std::vector<Vector3>& pts) : points(pts) {
    for (auto& p : const_cast<std::vector<Vector3>&>(points)) p.y = 0.0f;
}
