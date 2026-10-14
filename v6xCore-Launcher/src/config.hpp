#pragma once
#include "settings.hpp"
#include "ini.hpp"
#include "file_io.hpp"

namespace cs2 {
struct Offsets {
    Address entityList{}, localPawn{}, localController{}, viewMatrix{}, viewAngles{};
    Address sensitivity{}, sensitivityValue{}, health{}, team{}, life{}, sceneNode{};
    Address origin{}, absoluteOrigin{}, viewOffset{}, pawnHandle{}, playerName{};
    Address dormant{}, spottedState{}, spotted{}, immune{}, scoped{}, fovSensitivity{};
    std::wstring sourceDate, sourceCommit, retrievedDate, sourceUrl, provider, error, loadedPath;
    unsigned build{};
    bool valid = false;
    bool loadText(const std::string &text) {
        *this = {};
        try {
            auto ini = v6x::Ini::parse(text);
            auto global = [&](const char *key, Address &out) {
                out = static_cast<Address>(ini.integer("globals", key, 0x40000000));
            };
            auto field = [&](const char *key, Address &out) {
                out = static_cast<Address>(ini.integer("schema", key, 0x100000));
            };
            global("dwEntityList", entityList);
            global("dwLocalPlayerPawn", localPawn);
            global("dwLocalPlayerController", localController);
            global("dwViewMatrix", viewMatrix);
            global("dwViewAngles", viewAngles);
            global("dwSensitivity", sensitivity);
            field("dwSensitivity_sensitivity", sensitivityValue);
            field("m_iHealth", health);
            field("m_iTeamNum", team);
            field("m_lifeState", life);
            field("m_pGameSceneNode", sceneNode);
            field("m_vOldOrigin", origin);
            field("m_vecAbsOrigin", absoluteOrigin);
            field("m_vecViewOffset", viewOffset);
            field("m_hPlayerPawn", pawnHandle);
            field("m_iszPlayerName", playerName);
            field("m_bDormant", dormant);
            field("m_entitySpottedState", spottedState);
            field("m_bSpotted", spotted);
            field("m_bGunGameImmunity", immune);
            field("m_bIsScoped", scoped);
            field("m_flFOVSensitivityAdjust", fovSensitivity);
            sourceDate = v6x::wide(ini.get("metadata", "source_commit_date", "Unknown"));
            sourceCommit = v6x::wide(ini.get("metadata", "commit", "Unknown"));
            retrievedDate = v6x::wide(ini.get("metadata", "retrieved_on", "Unknown"));
            sourceUrl = v6x::wide(ini.get("metadata", "source", "Unknown"));
            provider = v6x::wide(ini.get("metadata", "provider", "Legacy snapshot"));
            auto buildText = ini.get("metadata", "build");
            if (!buildText.empty() && buildText != "0")
                build = static_cast<unsigned>(ini.integer("metadata", "build", 10000000));
            valid = true;
        } catch (const std::exception &e) {
            error = v6x::wide(e.what());
            valid = false;
        }
        return valid;
    }
    bool load(const std::wstring &path) {
        try {
            bool ok = loadText(v6x::readBytes(std::filesystem::path(path)));
            loadedPath = path;
            return ok;
        } catch (const std::exception &e) {
            *this = {};
            loadedPath = path;
            error = v6x::wide(e.what());
            return false;
        }
    }
};
} // namespace cs2
