#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

namespace cs2 {
constexpr float pi = 3.14159265358979323846f;
constexpr float rad(float degrees) {
    return degrees * pi / 180.0f;
}
constexpr float deg(float radians) {
    return radians * 180.0f / pi;
}
struct Vec2 {
    float x{}, y{};
};
struct Vec3 {
    float x{}, y{}, z{};
    Vec3 operator+(Vec3 b) const {
        return {x + b.x, y + b.y, z + b.z};
    }
    Vec3 operator-(Vec3 b) const {
        return {x - b.x, y - b.y, z - b.z};
    }
    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    bool sane(float limit = 131072.0f) const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::abs(x) < limit &&
               std::abs(y) < limit && std::abs(z) < limit;
    }
};
struct Matrix4 {
    float m[4][4]{};
};

inline float normalizeYaw(float angle) {
    return std::isfinite(angle) ? std::remainder(angle, 360.0f) : 0.0f;
}
inline std::optional<Vec2> project(Vec3 p, const Matrix4 &v, int width, int height) {
    if (!p.sane() || width <= 0 || height <= 0)
        return {};
    const float w = v.m[3][0] * p.x + v.m[3][1] * p.y + v.m[3][2] * p.z + v.m[3][3];
    if (!std::isfinite(w) || w < 0.01f)
        return {};
    const float x = (v.m[0][0] * p.x + v.m[0][1] * p.y + v.m[0][2] * p.z + v.m[0][3]) / w;
    const float y = (v.m[1][0] * p.x + v.m[1][1] * p.y + v.m[1][2] * p.z + v.m[1][3]) / w;
    if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x) > 8 || std::abs(y) > 8)
        return {};
    return Vec2{(x + 1.0f) * width * 0.5f, (1.0f - y) * height * 0.5f};
}
inline std::optional<Vec2> lookAt(Vec3 eye, Vec3 target) {
    const Vec3 d = target - eye;
    if (!d.sane() || d.length() < 0.001f)
        return {};
    return Vec2{deg(-std::atan2(d.z, std::hypot(d.x, d.y))), deg(std::atan2(d.y, d.x))};
}
inline Vec2 angleDelta(Vec2 desired, Vec2 current) {
    return {std::clamp(desired.x - current.x, -180.0f, 180.0f), normalizeYaw(desired.y - current.y)};
}
inline float angularDistance(Vec2 delta) {
    return std::hypot(delta.x, delta.y);
}
inline float smoothingAlpha(float smoothing, float dt) {
    if (smoothing <= 1.0f)
        return 1.0f;
    return 1.0f - std::pow(1.0f - 1.0f / smoothing, std::clamp(dt, 0.001f, 0.1f) * 60.0f);
}
inline Vec2 mouseCounts(Vec2 delta, float sensitivity, float yawScale, float pitchScale, float smoothing,
                        float dt) {
    if (!std::isfinite(sensitivity) || sensitivity <= 0 || yawScale <= 0 || pitchScale <= 0)
        return {};
    const float alpha = smoothingAlpha(smoothing, dt);
    return {-delta.y * alpha / (sensitivity * yawScale), delta.x * alpha / (sensitivity * pitchScale)};
}
} // namespace cs2
