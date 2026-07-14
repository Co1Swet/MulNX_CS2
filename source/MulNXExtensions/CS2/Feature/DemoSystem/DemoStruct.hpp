#pragma once
#include <string>
#include <vector>
#include <map>
using Steam64UID = uint64_t;

class RecordTask {
public:
    std::string desc;
    Steam64UID uid;
    int tickStart;
    int tickEnd;
    int tickMain;
    std::function<bool(int curTick, RecordTask* pRTask)>onPlaying;
};

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
        int roundNumber;
        Steam64UID killerSteamId;
        Steam64UID victimSteamId;
        Steam64UID assisterSteamId;
        std::string weaponName;
    };

    struct PlayerRoundInfo {
        std::vector<KillEvent> killEvents;
        std::optional<KillEvent> Bekilled;
    };

    struct Player {
        Steam64UID steamId;
        std::string name;
        std::string crosshairShareCode;
        bool isTeamA;

        int killCount;
        int deathCount;
        int assistCount;

        std::map<int, PlayerRoundInfo>roundInfo;
    };

    struct Info {
        std::string demoFileName;
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