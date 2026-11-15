#pragma once
#include <Intro/Intros.hpp>
namespace Demo {
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
namespace Demo::Record {
    struct Task {
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
    class Token {
        std::move_only_function<void(Token*)> callback = nullptr;
    public:
        Token() = delete;
        Token(std::move_only_function<void(Token*)> callback) :callback(std::move(callback)) {}
        ~Token() { callback(this); }

        std::chrono::system_clock::time_point createdAt;
        std::string createdAtStr;
    };
    struct Pass {
        std::shared_ptr<Token> token = nullptr;
        std::string kPath{};
        std::string demoFileName{};
        std::vector<Task> rTasks{};
    };
}

class DemoSystem final :public CSModule<DemoSystem> {
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

template<typename T>
class DemModuleMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    DemoSystem* pDemSys = nullptr;
    DemModuleMixin() {
        This()->preInits.push_back([this]() {
            This()->pDemSys = This()->FindModule<DemoSystem>();
            return true;
            });
    }
};

template <typename T>
class DemModule :public CSModule<T>, public DemModuleMixin<T> {
protected:
    // 立即从有效消息转换出信息，并摆脱消息本身生命周期
    inline std::expected <
        std::tuple<std::string, std::string, fs::path>,
        std::tuple<std::string, std::string, std::string>> ParseDemoPath(MulNX::Message& msg) {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        if (!pExt) {
            return std::unexpected(std::make_tuple(std::string(), std::string(), "消息中缺少 NetExt"));
        }
        const auto& key = pExt->str1;
        const auto& demoName = pExt->str2;
        auto demoPath = this->Path()->PathGetFromKey(key) / (demoName + ".dem");
        // 确保可拼接为绝对路径
        if (!demoPath.is_absolute()) {
            return std::unexpected(std::make_tuple(key, demoName, "无法拼接为绝对路径"));
        }
        return std::make_tuple(key, demoName, demoPath);
    }
    inline MulNX::Message MakeDemoInfoErrorNotify(const std::string& key, const std::string& demoName) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Info/Notify"_hash);
        auto&& [succ] = msg.Access<int>();
        succ = 1;
        rp->str1 = key;
        rp->str2 = demoName;
        return msg;
    }
};