#pragma once
#include "../vendor/nlohmann/json.hpp"
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace v6x {
using Json = nlohmann::json;
inline constexpr std::array<const char *, 6> globalNames = {
    "dwEntityList", "dwLocalPlayerPawn", "dwLocalPlayerController",
    "dwViewMatrix", "dwViewAngles",      "dwSensitivity"};
struct Field {
    const char *name;
    const char *type;
};
inline constexpr std::array<Field, 15> fields = {{{"m_iHealth", "C_BaseEntity"},
                                                  {"m_iTeamNum", "C_BaseEntity"},
                                                  {"m_lifeState", "C_BaseEntity"},
                                                  {"m_pGameSceneNode", "C_BaseEntity"},
                                                  {"m_vOldOrigin", "C_BasePlayerPawn"},
                                                  {"m_vecAbsOrigin", "CGameSceneNode"},
                                                  {"m_vecViewOffset", "C_BaseModelEntity"},
                                                  {"m_hPlayerPawn", "CCSPlayerController"},
                                                  {"m_iszPlayerName", "CBasePlayerController"},
                                                  {"m_bDormant", "CGameSceneNode"},
                                                  {"m_entitySpottedState", "C_CSPlayerPawn"},
                                                  {"m_bSpotted", "EntitySpottedState_t"},
                                                  {"m_bGunGameImmunity", "C_CSPlayerPawn"},
                                                  {"m_bIsScoped", "C_CSPlayerPawn"},
                                                  {"m_flFOVSensitivityAdjust", "C_BasePlayerPawn"}}};
inline std::uint64_t checkedNumber(const Json &raw, std::uint64_t maximum) {
    const Json &v = raw.is_object() ? raw.at("offset") : raw;
    if (!v.is_number_integer())
        throw std::runtime_error("Offset is not an integer");
    if (v.is_number_unsigned()) {
        auto n = v.get<std::uint64_t>();
        if (n && n <= maximum)
            return n;
    } else {
        auto n = v.get<std::int64_t>();
        if (n > 0 && static_cast<std::uint64_t>(n) <= maximum)
            return static_cast<std::uint64_t>(n);
    }
    throw std::runtime_error("Offset value outside allowed range");
}
inline bool validCommit(const std::string &s) {
    return s.size() == 40 && s.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
}
struct SnapshotMeta {
    std::string source, provider, commit, date, retrieved;
    unsigned build{};
};
inline std::string makeOffsetIni(const Json &globals, const Json &schema, const SnapshotMeta &meta) {
    if (!validCommit(meta.commit))
        throw std::runtime_error("Invalid source commit");
    for (const auto &v : {meta.source, meta.provider, meta.commit, meta.date, meta.retrieved})
        if (v.empty() || v.size() > 256 || v.find_first_of("\r\n[]") != std::string::npos)
            throw std::runtime_error("Invalid snapshot metadata");
    const auto &client = globals.at("client.dll");
    const auto &classes = schema.at("client.dll").at("classes");
    std::ostringstream ini;
    ini << "; Validated source data. Live CS2 compatibility remains unverified.\n[metadata]\nsource = "
        << meta.source << "\nprovider = " << meta.provider << "\ncommit = " << meta.commit
        << "\nsource_commit_date = " << meta.date << "\nretrieved_on = " << meta.retrieved
        << "\nbuild = " << meta.build << "\n\n[globals]\n";
    auto emit = [&](const char *key, const Json &value, std::uint64_t maximum) {
        ini << key << " = 0x" << std::hex << checkedNumber(value, maximum) << std::dec << '\n';
    };
    for (auto name : globalNames)
        emit(name, client.at(name), 0x40000000);
    ini << "\n[schema]\n";
    emit("dwSensitivity_sensitivity", client.at("dwSensitivity_sensitivity"), 0x100000);
    for (const auto &field : fields)
        emit(field.name, classes.at(field.type).at("fields").at(field.name), 0x100000);
    return ini.str();
}
} // namespace v6x
