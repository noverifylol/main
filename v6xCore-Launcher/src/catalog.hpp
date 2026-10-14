#pragma once
#include <array>
#include <string>

namespace v6x {
struct GameDefinition {
    const char *id;
    const char *title;
    const char *category;
    int coverResource;
    bool available;
    const char *appName;
    const char *ownerId;
    const char *authVersion;
};
// Application identifiers are public configuration, not authentication secrets.
// Each future game has its own configuration slot; none reuse the CS2 application.
inline constexpr std::array<GameDefinition, 8> catalog = {
    {{"cs2", "Counter-Strike 2", "TACTICAL / EXTERNAL", 501, true, "Counter Strike 2", "2goY8wtVEd", "1.0"},
     {"fortnite", "Fortnite", "BATTLE ROYALE", 502, false, "", "", ""},
     {"retrac", "Project Retrac", "CLASSIC BATTLE ROYALE", 503, false, "", "", ""},
     {"valorant", "Valorant", "TACTICAL SHOOTER", 504, false, "", "", ""},
     {"roblox", "Roblox", "SANDBOX", 505, false, "", "", ""},
     {"siege", "Rainbow Six Siege", "TACTICAL SHOOTER", 506, false, "", "", ""},
     {"warzone", "Call of Duty: Warzone", "BATTLE ROYALE", 507, false, "", "", ""},
     {"pubg", "PUBG: Battlegrounds", "BATTLE ROYALE", 508, false, "", "", ""}}};
inline constexpr const char *discordUrl = "https://discord.gg/v6xcore";
} // namespace v6x
