#pragma once

#include <string>
#include <vector>
#include <map>
using Steam64UID = uint64_t;

namespace Demo {
    struct Team {
        std::string name;
        std::string letter;
        int score;
        int scoreFirstHalf;
        int scoreSecondHalf;
        int currentSide;
    };

    struct KillEvent {
        int tick;
        Steam64UID killerSteamId;
        Steam64UID victimSteamId;
        Steam64UID assisterSteamId;
        std::string weaponName;
    };

    struct Player {
        Steam64UID steamId;
        std::string name;
        bool isTeamA;

        int killCount;
        int deathCount;
        int assistCount;

        std::vector<KillEvent> killEvents;
    };

    struct Info {
        std::string mapName;
        int tickCount;
        Team teamA;
        Team teamB;
        std::map<Steam64UID, Player> players;

        const std::string& GetPlayerName(Steam64UID steamId) const {
            auto it = players.find(steamId);
            if (it != players.end()) {
                return it->second.name;
            }
            static const std::string unknown = "N/A";
            return unknown;
        }
    };

}