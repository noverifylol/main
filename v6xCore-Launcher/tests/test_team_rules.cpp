#include "../src/team_rules.hpp"
#include "../src/frame_clock.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
    using namespace cs2;
    assert(sameSide(2, 2, false));
    assert(!sameSide(2, 3, false));
    assert(!sameSide(2, 2, true));
    assert(!displayPlayer(2, 2, false, false));
    assert(displayPlayer(2, 2, false, true));
    assert(displayPlayer(2, 2, true, false));
    assert(displayPlayer(2, 3, true, false));
    assert(!targetPlayer(2, 2, false, false));
    assert(targetPlayer(2, 2, false, true));
    assert(targetPlayer(2, 2, true, false));
    assert(targetPlayer(2, 3, false, false));
    v6x::FrameClock clock;
    assert(clock.due(1.0, 240));
    assert(!clock.due(1.001, 240));
    assert(clock.due(1.005, 240));
    assert(clock.due(2.0, 240));
    assert(!clock.due(2.0001, 240));
    assert(clock.due(3.0, 10000));
    assert(clock.next >= 3.0 + 1.0 / 360.0 - .000001);
    std::cout << "PASS: FFA display/target classification, opt-in teammate targeting, frame deadlines and no "
                 "catch-up burst\n";
}
