#pragma once
#include <Intro/CSModuleBase.hpp>

namespace Demo {
    struct RecordTask {
        enum class Type :int {
            None,
            Normal, // 给击杀分配这个，让合并智能一点
            Death,
            Campath
        };
        // demo文件所在的key
        std::string kPath{};
        // demo文件的裸名称
        std::string demoName{};
        // 任务描述
        std::string desc{};
        // 目标SteamUID
        Steam64UID uid = 0;
        // 录制类型，和优化有关
        Type type = Type::None;
        // 开始录制tick
        int tickStart{};
        // 结束录制tick
        int tickEnd{};
        // 回合编号
        int roundNumber{};
        // uid这个回合的击杀总量
        int roundKill{};
        // 这是回合的第几个击杀
        int killIndex{};
        // 没有扩展名的文件名，不包括路径
        std::string fileNameNoExt{};
    };

    struct RecordPass {
        std::string kPath{};
        std::string demoFileName{};
        std::vector<RecordTask> rTasks{};
    };

    struct Team {
        std::string name{};
        std::string letter{};
        int score{};
        int scoreFirstHalf{};
        int scoreSecondHalf{};
        int currentSide{};
    };

    struct KillEvent {
        int tick{};
        int roundNumber{};
        Steam64UID killerSteamId{};
        Steam64UID victimSteamId{};
        Steam64UID assisterSteamId{};
        std::string weaponName{};
    };

    struct PlayerRoundInfo {
        std::vector<KillEvent> killEvents{};
        std::optional<KillEvent> Bekilled{};
    };

    struct Player {
        Steam64UID steamId{};
        std::string name{};
        std::string crosshairShareCode{};
        bool isTeamA{};

        int killCount{};
        int deathCount{};
        int assistCount{};

        std::map<int, PlayerRoundInfo>roundInfo{};
    };

    struct Round {
        int number{};
        int startTick{};
        int freezeTimeEndTick{};
        int endTick{};
        int endOfficiallyTick{};
        int duration{};
    };

    struct Info {
        std::string kDir{};
        std::string demoFileName{};
        std::string mapName{};
        int tickCount{};
        Team teamA{};
        Team teamB{};
        std::map<Steam64UID, Player> players{};
        std::vector<Round> rounds{};

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

class DemoSystem final :public CSModuleBase {
    std::map<std::string, // 先kDir，再demoName
        std::map<std::string, std::shared_ptr<Demo::Info>>> demos;
    
    std::atomic<std::shared_ptr<std::pair<std::string, std::string>>>currentOperatingDemo = nullptr;
    std::atomic<std::shared_ptr<std::pair<std::string, std::string>>>currentPlayingDemo = nullptr;

    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);

    void OnInfoLoad(MulNX::Message& msg);
public:
    std::shared_ptr<std::pair<std::string, std::string>> GetCurrentOperatingDemo()const;
    std::shared_ptr<std::pair<std::string, std::string>> GetCurrentPlayingDemo()const;
    std::shared_ptr<const Demo::Info> GetDemoInfo(const std::string& kDir, const std::string& demoName)const;
    std::shared_ptr<const Demo::Info> GetCurOpDemoInfo()const;
};