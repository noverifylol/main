#pragma once
namespace cs2 {
inline bool sameSide(int localTeam, int playerTeam, bool freeForAll) {
    return !freeForAll && localTeam == playerTeam;
}
inline bool displayPlayer(int localTeam, int playerTeam, bool freeForAll, bool showTeammates) {
    return !sameSide(localTeam, playerTeam, freeForAll) || showTeammates;
}
inline bool targetPlayer(int localTeam, int playerTeam, bool freeForAll, bool targetTeammates) {
    return !sameSide(localTeam, playerTeam, freeForAll) || targetTeammates;
}
} // namespace cs2
