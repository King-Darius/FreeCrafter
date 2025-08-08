#include "Solid.h"

Solid::Solid(const std::vector<Vector3>& baseProfile, float height) {
    std::vector<Vector3> profile = baseProfile;
    if (profile.empty()) return;
    Vector3 first = profile.front();
    Vector3 last = profile.back();
    if (first.x != last.x || first.z != last.z) profile.push_back(first);
    size_t N = profile.size();
    if (N < 2) return;

    vertices.reserve(N * 2);
    for (size_t i = 0; i < N; ++i) vertices.emplace_back(profile[i].x, 0.0f, profile[i].z);
    for (size_t i = 0; i < N; ++i) vertices.emplace_back(profile[i].x, height, profile[i].z);

    faces.clear(); faces.reserve(N + 2);
    for (size_t i = 0; i < N - 1; ++i) {
        Face f; int b0=(int)i, b1=(int)(i+1), t0=(int)(i+N), t1=(int)(i+1+N);
        f.indices = { b0, b1, t1, t0 }; faces.push_back(f);
    }
    Face bottom; for (size_t i=0;i<N-1;++i) bottom.indices.push_back((int)i); faces.push_back(bottom);
    Face top;    for (size_t i=0;i<N-1;++i) top.indices.push_back((int)(N+i)); faces.push_back(top);
}
