#pragma once
#include <cmath>

struct Vec3 {
    float x, y, z;

    Vec3(float x = 0.f, float y = 0.f, float z = 0.f) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float length2() const { return x * x + y * y + z * z; }

    Vec3 normalized() const {
        float l = length();
        if (l < 1e-6f) return {0, 0, 0};
        return *this / l;
    }

    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    float dist(const Vec3& o) const { return (*this - o).length(); }
    float dist2(const Vec3& o) const { return (*this - o).length2(); }

    // Flatten to 2D (zero out z)
    Vec3 flat() const { return {x, y, 0.f}; }
};

inline float clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float sign(float v) {
    return v >= 0.f ? 1.f : -1.f;
}

// Angle in radians between -PI and PI
inline float angle2D(const Vec3& a, const Vec3& b) {
    return std::atan2(a.x * b.y - a.y * b.x, a.x * b.x + a.y * b.y);
}
