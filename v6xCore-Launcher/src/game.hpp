#pragma once
#include "config.hpp"
#include "math.hpp"
#include "team_rules.hpp"
#include <array>
#include <cstdio>
#include <vector>

namespace cs2 {
struct Player {
    Address pawn{};
    int health{}, team{};
    Vec3 feet{}, aimPoint{}, top{};
    bool radarKnown{}, immune{};
    std::wstring name;
};
struct Frame {
    bool valid{};
    int team{};
    Vec3 eye{};
    Vec2 angles{};
    Matrix4 matrix{};
    float sensitivity{};
    bool scoped{};
    std::vector<Player> players;
};

inline std::wstring utf8Name(const char *data) {
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data, -1, nullptr, 0);
    if (length <= 1 || length > 256)
        return L"Player";
    std::wstring text(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data, -1, text.data(), length);
    text.pop_back();
    for (auto &c : text)
        if (c < 32)
            c = L' ';
    return text;
}

class GameReader {
    Process &process_;
    Offsets &offsets_;
    Address lastList_{}, lastPawn_{};
    std::size_t stride_{};

    Address entity(Address list, std::uint32_t handle, std::size_t stride) const {
        if (handle == 0 || handle == 0xffffffffu)
            return 0;
        const auto index = handle & 0x7fffu;
        if (index == 0)
            return 0;
        const Address chunk = process_.pointer(list + 0x10 + 8 * (index >> 9));
        return chunk ? process_.pointer(chunk + stride * (index & 0x1ff)) : 0;
    }
    bool eyeOffset(Address pawn, Vec3 &value) const {
        if (!process_.read(pawn + offsets_.viewOffset, value))
            return false;
        return value.sane(128) && std::abs(value.x) < 32 && std::abs(value.y) < 32 && value.z >= 20 &&
               value.z <= 90;
    }

  public:
    std::wstring status = L"Waiting for CS2";
    GameReader(Process &p, Offsets &o) : process_(p), offsets_(o) {}
    void reset() {
        lastList_ = 0;
        lastPawn_ = 0;
        stride_ = 0;
    }
    std::size_t stride() const {
        return stride_;
    }

    bool sample(Frame &frame) {
        frame = {};
        if (!offsets_.valid) {
            status = offsets_.error;
            return false;
        }
        const Address base = process_.client();
        if (!base || !process_.alive()) {
            status = L"CS2 is not attached";
            reset();
            return false;
        }
        for (Address global : {offsets_.entityList, offsets_.localPawn, offsets_.localController,
                               offsets_.viewMatrix, offsets_.viewAngles, offsets_.sensitivity}) {
            if (global + sizeof(Matrix4) >= process_.clientSize()) {
                status = L"Offsets exceed client.dll; update offsets.ini";
                return false;
            }
        }
        const Address list = process_.pointer(base + offsets_.entityList);
        const Address local = process_.pointer(base + offsets_.localPawn);
        const Address controller = process_.pointer(base + offsets_.localController);
        if (!list || !local || !controller) {
            status = L"No local player. Join a map; otherwise check offsets.";
            reset();
            return false;
        }
        int localHealth{};
        std::uint8_t localTeam{}, life{};
        if (!process_.read(local + offsets_.health, localHealth) ||
            !process_.read(local + offsets_.team, localTeam) || !process_.read(local + offsets_.life, life)) {
            status = L"Cannot read local player; check access / offsets";
            return false;
        }
        if (localHealth <= 0 || life != 0) {
            status = L"Local player is not alive";
            return false;
        }
        if (localHealth > 1000 || (localTeam != 2 && localTeam != 3)) {
            status = L"Invalid player data; check offsets";
            return false;
        }
        std::uint32_t localHandle{};
        if (!process_.read(controller + offsets_.pawnHandle, localHandle)) {
            status = L"Cannot read local pawn handle";
            return false;
        }
        // Validate layout against an independently known local pawn; never assume a guessed stride.
        if (list != lastList_ || local != lastPawn_ || !stride_ ||
            entity(list, localHandle, stride_) != local) {
            stride_ = 0;
            for (std::size_t candidate : {0x70u, 0x78u}) {
                if (entity(list, localHandle, candidate) == local) {
                    stride_ = candidate;
                    break;
                }
            }
            if (!stride_) {
                status = L"Unsupported entity layout; stopped (no guessed reads)";
                return false;
            }
            lastList_ = list;
            lastPawn_ = local;
        }
        Vec3 origin{}, view{}, angles{};
        if (!process_.read(local + offsets_.origin, origin) || !origin.sane() || !eyeOffset(local, view) ||
            !process_.read(base + offsets_.viewAngles, angles) || !angles.sane(10000) ||
            std::abs(angles.x) > 90 || std::abs(angles.y) > 360 ||
            !process_.read(base + offsets_.viewMatrix, frame.matrix)) {
            status = L"Invalid camera data; check offsets";
            return false;
        }
        float matrixEnergy{};
        for (const auto &row : frame.matrix.m)
            for (float f : row) {
                if (!std::isfinite(f) || std::abs(f) > 1e7f) {
                    status = L"Invalid view matrix; input stopped";
                    return false;
                }
                matrixEnergy += std::abs(f);
            }
        if (matrixEnergy < 0.1f) {
            status = L"View matrix not ready";
            return false;
        }
        frame.eye = origin + view;
        frame.angles = {angles.x, angles.y};
        frame.team = localTeam;
        std::uint8_t scoped{};
        process_.read(local + offsets_.scoped, scoped);
        frame.scoped = scoped != 0;
        float sensitivity{};
        const Address sensitivityObject = process_.pointer(base + offsets_.sensitivity);
        if (sensitivityObject && process_.read(sensitivityObject + offsets_.sensitivityValue, sensitivity) &&
            std::isfinite(sensitivity) && sensitivity >= 0.01f && sensitivity <= 100) {
            if (frame.scoped) {
                float adjust{};
                if (!process_.read(local + offsets_.fovSensitivity, adjust) || !std::isfinite(adjust) ||
                    adjust < 0.01f || adjust > 2.0f)
                    sensitivity = 0;
                else
                    sensitivity *= adjust;
            }
            frame.sensitivity = sensitivity;
        }
        frame.players.reserve(64);
        for (std::uint32_t slot = 1; slot <= 64; ++slot) {
            const Address playerController = entity(list, slot, stride_);
            if (!playerController || playerController == controller)
                continue;
            std::uint32_t pawnHandle{};
            if (!process_.read(playerController + offsets_.pawnHandle, pawnHandle))
                continue;
            const Address pawn = entity(list, pawnHandle, stride_);
            if (!pawn || pawn == local)
                continue;
            Player player;
            player.pawn = pawn;
            std::uint8_t team{}, state{}, dormant{}, spotted{}, immune{1};
            if (!process_.read(pawn + offsets_.health, player.health) || player.health <= 0 ||
                player.health > 1000 || !process_.read(pawn + offsets_.team, team) ||
                (team != 2 && team != 3) || !process_.read(pawn + offsets_.life, state) || state != 0)
                continue;
            const Address node = process_.pointer(pawn + offsets_.sceneNode);
            if (!node || !process_.read(node + offsets_.dormant, dormant) || dormant ||
                !process_.read(node + offsets_.absoluteOrigin, player.feet) || !player.feet.sane())
                continue;
            Vec3 playerView{};
            if (!eyeOffset(pawn, playerView))
                continue;
            player.team = team;
            player.aimPoint = player.feet + playerView - Vec3{0, 0, 3};
            player.top = player.feet + Vec3{0, 0, playerView.z + 8};
            process_.read(pawn + offsets_.spottedState + offsets_.spotted, spotted);
            process_.read(pawn + offsets_.immune, immune);
            player.radarKnown = spotted != 0;
            player.immune = immune != 0;
            char name[128]{};
            if (process_.bytes(playerController + offsets_.playerName, name, sizeof(name) - 1))
                player.name = utf8Name(name);
            else
                player.name = L"Player " + std::to_wstring(slot);
            frame.players.push_back(std::move(player));
        }
        frame.valid = true;
        status = L"Live data: " + std::to_wstring(frame.players.size()) + L" players";
        if (frame.sensitivity <= 0)
            status += L" | Aim disabled: sensitivity invalid";
        return true;
    }
};

class AimController {
    Address locked_{};
    Vec2 remainder_{};

  public:
    Address target{};
    bool inputRejected{};
    void reset() {
        locked_ = 0;
        remainder_ = {};
        target = 0;
        inputRejected = false;
    }
    const Player *choose(const Frame &frame, const Config &cfg) const {
        const Player *best = nullptr;
        float score = cfg.fov;
        for (const auto &player : frame.players) {
            if (!targetPlayer(frame.team, player.team, cfg.freeForAll, cfg.targetTeammates) ||
                player.immune || (cfg.spottedOnly && !player.radarKnown))
                continue;
            const auto angles = lookAt(frame.eye, player.aimPoint);
            if (!angles)
                continue;
            const float distance = angularDistance(angleDelta(*angles, frame.angles));
            if (distance > cfg.fov)
                continue;
            if (player.pawn == locked_)
                return &player;
            if (distance < score) {
                score = distance;
                best = &player;
            }
        }
        return best;
    }
    void update(const Frame &frame, const Config &cfg, bool gameActive, float dt) {
        if (!gameActive || !frame.valid || !cfg.aim || frame.sensitivity <= 0 ||
            !(GetAsyncKeyState(cfg.aimKey) & 0x8000)) {
            reset();
            return;
        }
        const auto *player = choose(frame, cfg);
        if (!player) {
            reset();
            return;
        }
        if (locked_ != player->pawn)
            remainder_ = {};
        locked_ = player->pawn;
        target = locked_;
        const auto desired = lookAt(frame.eye, player->aimPoint);
        if (!desired) {
            reset();
            return;
        }
        const Vec2 counts = mouseCounts(angleDelta(*desired, frame.angles), frame.sensitivity, cfg.yawScale,
                                        cfg.pitchScale, cfg.smoothing, dt);
        if (!std::isfinite(counts.x) || !std::isfinite(counts.y)) {
            reset();
            return;
        }
        const float x = std::clamp(counts.x + remainder_.x, -500.0f, 500.0f);
        const float y = std::clamp(counts.y + remainder_.y, -500.0f, 500.0f);
        const LONG dx = std::lround(x), dy = std::lround(y);
        remainder_ = {x - static_cast<float>(dx), y - static_cast<float>(dy)};
        if (!dx && !dy)
            return;
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dx = dx;
        input.mi.dy = dy;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        // Ordinary, explicitly held synthetic mouse input. No game-memory writes and no auto-fire.
        inputRejected = SendInput(1, &input, sizeof(input)) != 1;
        if (inputRejected)
            remainder_ = {};
    }
};
} // namespace cs2
