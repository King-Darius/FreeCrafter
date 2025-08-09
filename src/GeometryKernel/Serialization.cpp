#include "Serialization.h"
#include "Curve.h"
#include "Solid.h"
#include <vector>

namespace GeometryIO {

void writeCurve(std::ostream& os, const Curve& curve) {
    const auto& pts = curve.getPoints();
    os << "Curve " << pts.size() << "\n";
    for (const auto& p : pts) {
        os << p.x << ' ' << p.y << ' ' << p.z << "\n";
    }
}

std::unique_ptr<Curve> readCurve(std::istream& is) {
    size_t count;
    if (!(is >> count)) return nullptr;
    std::vector<Vector3> pts(count);
    for (size_t i = 0; i < count; ++i) {
        is >> pts[i].x >> pts[i].y >> pts[i].z;
    }
    return std::make_unique<Curve>(pts);
}

void writeSolid(std::ostream& os, const Solid& solid) {
    const auto& verts = solid.getVertices();
    if (verts.empty()) {
        os << "Solid 0 0\n";
        return;
    }
    size_t N = verts.size() / 2;
    float height = verts[N].y;
    os << "Solid " << N << ' ' << height << "\n";
    for (size_t i = 0; i < N; ++i) {
        const auto& v = verts[i];
        os << v.x << ' ' << v.y << ' ' << v.z << "\n";
    }
}

std::unique_ptr<Solid> readSolid(std::istream& is) {
    size_t N;
    float height;
    if (!(is >> N >> height)) return nullptr;
    std::vector<Vector3> profile(N);
    for (size_t i = 0; i < N; ++i) {
        is >> profile[i].x >> profile[i].y >> profile[i].z;
    }
    return std::make_unique<Solid>(profile, height);
}

} // namespace GeometryIO

