#include "Serialization.h"
#include "Curve.h"
#include "Solid.h"
#include <vector>
#include <ostream>
#include <istream>

namespace GeometryIO {

void writeCurve(std::ostream& os, const Curve& curve) {
    const auto& pts = curve.getPoints();
    os << "Curve " << pts.size() << "\n";
    for (const auto& p : pts) {
        os << p.x << ' ' << p.y << ' ' << p.z << "\n";
    }
}

std::unique_ptr<Curve> readCurve(std::istream& is) {
    size_t n=0; is >> n;
    std::vector<Vector3> pts; pts.reserve(n);
    for (size_t i=0;i<n;++i) {
        Vector3 p; is >> p.x >> p.y >> p.z; pts.push_back(p);
    }
    return std::make_unique<Curve>(pts);
}

void writeSolid(std::ostream& os, const Solid& solid) {
    const auto& verts = solid.getVertices();
    size_t N = verts.size()/2; // base ring count
    os << "Solid " << N << ' ' << (N?verts[N].y:0.0f) << "\n";
    for (size_t i=0;i<N;++i) {
        const auto& p = verts[i];
        os << p.x << ' ' << 0.0f << ' ' << p.z << "\n";
    }
}

std::unique_ptr<Solid> readSolid(std::istream& is) {
    size_t n=0; float h=0.0f; is >> n >> h;
    std::vector<Vector3> base; base.reserve(n);
    for (size_t i=0;i<n;++i) {
        Vector3 p; is >> p.x >> p.y >> p.z; base.push_back(Vector3(p.x,0.0f,p.z));
    }
    return std::make_unique<Solid>(base, h);
}

} // namespace
