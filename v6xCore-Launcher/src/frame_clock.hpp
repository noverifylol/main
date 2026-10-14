#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
namespace v6x {
inline double secondsNow() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct FrameClock {
    double next{};
    bool due(double now, double hz) {
        hz = std::clamp(hz, 1.0, 360.0);
        if (now + 0.00005 < next)
            return false;
        next = now + 1.0 / hz;
        return true;
    }
};
} // namespace v6x
