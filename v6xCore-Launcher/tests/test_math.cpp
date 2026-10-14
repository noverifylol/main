#include "../src/math.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>

using namespace cs2;
bool near(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}
int main() {
    assert(near(normalizeYaw(181), -179));
    assert(near(normalizeYaw(-181), 179));
    assert(near(normalizeYaw(720), 0));
    auto east = lookAt({0, 0, 0}, {100, 0, 0});
    assert(east && near(east->x, 0) && near(east->y, 0));
    auto north = lookAt({0, 0, 0}, {0, 100, 0});
    assert(north && near(north->y, 90));
    auto above = lookAt({0, 0, 0}, {100, 0, 100});
    assert(above && near(above->x, -45));
    assert(!lookAt({0, 0, 0}, {0, 0, 0}));
    const auto wrapped = angleDelta({0, -179}, {0, 179});
    assert(near(wrapped.y, 2));
    Matrix4 identity{};
    for (int i = 0; i < 4; ++i)
        identity.m[i][i] = 1;
    auto center = project({0, 0, 0}, identity, 1920, 1080);
    assert(center && near(center->x, 960) && near(center->y, 540));
    auto corner = project({1, 1, 0}, identity, 1920, 1080);
    assert(corner && near(corner->x, 1920) && near(corner->y, 0));
    assert(!project({0, 0, 0}, identity, 0, 1080));
    identity.m[3][3] = -1;
    assert(!project({0, 0, 0}, identity, 1920, 1080));
    Matrix4 invalid{};
    invalid.m[3][3] = NAN;
    assert(!project({0, 0, 0}, invalid, 1920, 1080));
    Vec2 left = mouseCounts({0, 2.2f}, 1, 0.022f, 0.022f, 1, 1.0f / 60);
    assert(near(left.x, -100, 0.001f) && near(left.y, 0));
    Vec2 down = mouseCounts({2.2f, 0}, 1, 0.022f, 0.022f, 1, 1.0f / 60);
    assert(near(down.y, 100, 0.001f));
    assert(near(mouseCounts({2, 2}, 0, 0.022f, 0.022f, 1, 0.016f).x, 0));
    const float a60 = smoothingAlpha(6, 1.0f / 60), a120 = smoothingAlpha(6, 1.0f / 120);
    assert(near(1 - a60, (1 - a120) * (1 - a120)));
    assert(near(smoothingAlpha(1, 0.016f), 1));
    std::cout << "PASS: projection, angle wrapping, aim geometry, input signs and frame-rate-independent "
                 "smoothing\n";
}
