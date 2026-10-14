#pragma once
#include "memory.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace cs2 {
inline std::wstring iniString(const std::wstring &file, const wchar_t *section, const wchar_t *key,
                              const wchar_t *fallback = L"") {
    wchar_t buffer[512]{};
    GetPrivateProfileStringW(section, key, fallback, buffer, 512, file.c_str());
    return buffer;
}
inline float iniFloat(const std::wstring &file, const wchar_t *key, float fallback, float minimum,
                      float maximum) {
    const auto raw = iniString(file, L"settings", key);
    wchar_t *end{};
    const float v = std::wcstof(raw.c_str(), &end);
    return raw.empty() || end == raw.c_str() || *end || !std::isfinite(v) ? fallback
                                                                          : std::clamp(v, minimum, maximum);
}
struct Config {
    bool esp = true, aim = false, boxes = true, names = true, health = true, distance = true;
    bool teammates = false, fovRing = true, spottedOnly = false, healthText = true;
    bool snaplines = false, headMarker = false, outlines = true, watermark = true, crosshair = false;
    bool animation = true, showFps = true, autoConnect = true;
    bool freeForAll = false, targetTeammates = false;
    int sampleHz = 240;
    int boxStyle = 1, lineOrigin = 2, aimKey = VK_MENU, overlayFps = 120;
    float fov = 4, smoothing = 6, yawScale = 0.022f, pitchScale = 0.022f;
    float maxDistance = 150, thickness = 1.4f, opacity = 1, crosshairSize = 6, textSize = 14;
    std::array<float, 3> accent{.94f, .94f, .96f}, enemyColor{.94f, .94f, .96f};
    std::array<float, 3> allyColor{0.48f, 0.71f, 1.0f}, targetColor{0.47f, 0.92f, 0.73f};
    std::wstring path;
    void load(const std::wstring &p) {
        *this = Config{};
        path = p;
        auto b = [&](const wchar_t *key, bool &v) {
            v = GetPrivateProfileIntW(L"settings", key, v ? 1 : 0, path.c_str()) != 0;
        };
        b(L"esp", esp);
        b(L"boxes", boxes);
        b(L"names", names);
        b(L"health", health);
        b(L"distance", distance);
        b(L"teammates", teammates);
        b(L"fov_ring", fovRing);
        b(L"radar_known_only", spottedOnly);
        b(L"health_text", healthText);
        b(L"snaplines", snaplines);
        b(L"head_marker", headMarker);
        b(L"outlines", outlines);
        b(L"watermark", watermark);
        b(L"crosshair", crosshair);
        b(L"logo_animation", animation);
        b(L"show_fps", showFps);
        b(L"auto_connect", autoConnect);
        b(L"free_for_all", freeForAll);
        b(L"target_teammates", targetTeammates);
        fov = iniFloat(path, L"fov_degrees", 4, 1, 15);
        smoothing = iniFloat(path, L"smoothing", 6, 1, 20);
        yawScale = 0.022f;
        pitchScale = 0.022f;
        maxDistance = iniFloat(path, L"max_distance", 150, 5, 500);
        thickness = iniFloat(path, L"line_width", 1.4f, 1, 4);
        opacity = iniFloat(path, L"opacity", 1, 0.2f, 1);
        crosshairSize = iniFloat(path, L"crosshair_size", 6, 2, 20);
        textSize = iniFloat(path, L"text_size", 14, 10, 22);
        boxStyle = static_cast<int>(iniFloat(path, L"box_style", 1, 0, 2));
        lineOrigin = static_cast<int>(iniFloat(path, L"line_origin", 2, 0, 2));
        overlayFps = static_cast<int>(iniFloat(path, L"overlay_fps", 120, 30, 360));
        sampleHz = static_cast<int>(iniFloat(path, L"sample_hz", 240, 30, 360));
        const auto key = iniString(path, L"settings", L"hold_key", L"ALT");
        aimKey = key == L"RMB"      ? VK_RBUTTON
                 : key == L"MOUSE4" ? VK_XBUTTON1
                 : key == L"MOUSE5" ? VK_XBUTTON2
                 : key == L"SHIFT"  ? VK_LSHIFT
                                    : VK_MENU;
        auto color = [&](const wchar_t *prefix, std::array<float, 3> &v) {
            for (int i = 0; i < 3; ++i) {
                const auto k = std::wstring(prefix) + L"_" + std::to_wstring(i);
                v[i] = iniFloat(path, k.c_str(), v[i], 0, 1);
            }
        };
        color(L"accent", accent);
        color(L"enemy", enemyColor);
        color(L"ally", allyColor);
        color(L"target", targetColor);
        aim = false;
    }
    const wchar_t *keyName() const {
        return aimKey == VK_RBUTTON    ? L"RMB"
               : aimKey == VK_XBUTTON1 ? L"MOUSE4"
               : aimKey == VK_XBUTTON2 ? L"MOUSE5"
               : aimKey == VK_LSHIFT   ? L"SHIFT"
                                       : L"ALT";
    }
    bool saveAs(const std::wstring &destination) const {
        bool ok = true;
        auto put = [&](const wchar_t *k, const std::wstring &v) {
            if (!WritePrivateProfileStringW(L"settings", k, v.c_str(), destination.c_str()))
                ok = false;
        };
        auto b = [&](const wchar_t *k, bool v) { put(k, v ? L"1" : L"0"); };
        auto n = [&](const wchar_t *k, float v) { put(k, std::to_wstring(v)); };
        b(L"esp", esp);
        b(L"boxes", boxes);
        b(L"names", names);
        b(L"health", health);
        b(L"distance", distance);
        b(L"teammates", teammates);
        b(L"fov_ring", fovRing);
        b(L"radar_known_only", spottedOnly);
        b(L"health_text", healthText);
        b(L"snaplines", snaplines);
        b(L"head_marker", headMarker);
        b(L"outlines", outlines);
        b(L"watermark", watermark);
        b(L"crosshair", crosshair);
        b(L"logo_animation", animation);
        b(L"show_fps", showFps);
        b(L"auto_connect", autoConnect);
        b(L"free_for_all", freeForAll);
        b(L"target_teammates", targetTeammates);
        n(L"fov_degrees", fov);
        n(L"smoothing", smoothing);
        n(L"m_yaw", yawScale);
        n(L"m_pitch", pitchScale);
        n(L"max_distance", maxDistance);
        n(L"line_width", thickness);
        n(L"opacity", opacity);
        n(L"crosshair_size", crosshairSize);
        n(L"text_size", textSize);
        n(L"box_style", static_cast<float>(boxStyle));
        n(L"line_origin", static_cast<float>(lineOrigin));
        n(L"overlay_fps", static_cast<float>(overlayFps));
        n(L"sample_hz", static_cast<float>(sampleHz));
        put(L"hold_key", keyName());
        auto color = [&](const wchar_t *prefix, const std::array<float, 3> &v) {
            for (int i = 0; i < 3; ++i) {
                const auto k = std::wstring(prefix) + L"_" + std::to_wstring(i);
                n(k.c_str(), v[i]);
            }
        };
        color(L"accent", accent);
        color(L"enemy", enemyColor);
        color(L"ally", allyColor);
        color(L"target", targetColor);
        return ok;
    }
    bool save() const {
        return !path.empty() && saveAs(path);
    }
};
} // namespace cs2
