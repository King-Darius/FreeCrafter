#pragma once
#include <cmath>

class Vector3 {
public:
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    float lengthSquared() const { return x*x + y*y + z*z; }
    Vector3 normalized() const { float l = length(); return l>1e-6f ? Vector3(x/l,y/l,z/l) : Vector3(); }
    float dot(const Vector3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vector3 cross(const Vector3& o) const { return Vector3(y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x); }
    Vector3 operator+(const Vector3& o) const { return Vector3(x+o.x, y+o.y, z+o.z); }
    Vector3 operator-(const Vector3& o) const { return Vector3(x-o.x, y-o.y, z-o.z); }
    Vector3 operator*(float s) const { return Vector3(x*s, y*s, z*s); }
    Vector3 operator/(float s) const { return Vector3(x/s, y/s, z/s); }
    Vector3& operator+=(const Vector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vector3& operator-=(const Vector3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
};
