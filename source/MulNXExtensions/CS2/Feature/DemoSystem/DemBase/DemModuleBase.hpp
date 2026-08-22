#pragma once
#include "DemState/DemState.hpp"

class RecordTask {
public:
    // 任务描述
    std::string desc{};
    // 目标SteamUID
    Steam64UID uid{};
    // 开始录制tick
    int tickStart{};
    // 结束录制tick
    int tickEnd{};
    // 回合开始tick（用于跳转到起点）
    int tickRoundStart{};

    // 一个在录制时每帧调用的回调
    std::function<bool(int curTick, RecordTask* pRTask)>onPlaying{};
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

template<typename T>
class DemModuleMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    DemState* pDemState = nullptr;
    DemModuleMixin() {
        This()->preInits.push_back([this]() {
            This()->pDemState = This()->FindModule<DemState>("DemState");
            return true;
            });
    }
};

class DemModuleBase :public CSModuleBase, public DemModuleMixin<DemModuleBase> {};