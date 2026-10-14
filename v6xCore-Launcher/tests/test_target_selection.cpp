#include "../src/game.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
    cs2::Frame frame;
    frame.valid = true;
    frame.team = 2;
    frame.eye = {0, 0, 0};
    frame.angles = {0, 0};
    cs2::Player ally;
    ally.pawn = 0x123456;
    ally.team = 2;
    ally.aimPoint = {100, 0, 0};
    ally.health = 100;
    ally.radarKnown = true;
    frame.players.push_back(ally);
    cs2::Config cfg;
    cs2::AimController controller;
    assert(!controller.choose(frame, cfg));
    cfg.targetTeammates = true;
    assert(controller.choose(frame, cfg));
    cfg.targetTeammates = false;
    cfg.freeForAll = true;
    assert(controller.choose(frame, cfg));
    frame.players[0].immune = true;
    assert(!controller.choose(frame, cfg));
    frame.players[0].immune = false;
    frame.players[0].radarKnown = false;
    cfg.spottedOnly = true;
    assert(!controller.choose(frame, cfg));
    std::cout << "PASS: target selection respects team mode, FFA, opt-in teammate selection, immunity and "
                 "radar filter; no process reads or mouse input used\n";
}
