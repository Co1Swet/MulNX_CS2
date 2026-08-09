#pragma once
#include <Intro/CSController/CSController.hpp>
#include <MulNXUtils/WinExt/HookMixin.hpp>
#include <Intro/HookConsole/CSConMixin.hpp>

class PlayerHub;
class HookView;
class TimeController;
class HookConsole;
class CS2Hash;

class ICSModule {
public:
    ~ICSModule() = default;

    virtual void OnItBegin() {};
    virtual void OnItEntity(int index, CS2::C_BaseEntity*) {};
    virtual void OnItPlayer(int index, CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*) {};
    virtual void OnItEnd() {};
};

template <typename T>
class CSModuleMixin : public ICSModule, public HookMixin<T>, public CSConMixin<T> {
    class CS2Paths {
    public:
        // Counter-Strike Global Offensive
        std::filesystem::path root{};
        std::filesystem::path exe{};
        std::filesystem::path config{};
        std::filesystem::path demo{};

        CS2Paths() {
            // 获取 cs2.exe 的完整路径（进程主模块）
            WCHAR cs2Path[MAX_PATH] = { 0 };
            GetModuleFileNameW(nullptr, cs2Path, MAX_PATH);// 注意这里不传句柄，拿CS2的exe的位置
            this->exe = std::filesystem::path(cs2Path);
            this->root = this->exe.parent_path().parent_path().parent_path().parent_path();
            this->demo = this->root / "game" / "csgo";
            this->config = this->root / "game" / "csgo" / "cfg";
        }
        static CS2Paths* Get() {
            static CS2Paths CS2Paths{};
            return &CS2Paths;
        }
    };
    T* This() { return static_cast<T*>(this); }
    const T* This() const { return static_cast<const T*>(this); }
public:
    CSController* CS2 = nullptr;
    HookView* CS2View = nullptr;
    TimeController* CS2Time = nullptr;
    PlayerHub* Hub = nullptr;
    HookConsole* CS2Con = nullptr;
    CS2Paths* CS2Paths = nullptr;
    CS2Hash* CS2Hashs = nullptr;
protected:
    bool participateIt = false;
    CSModuleMixin() {
        This()->preInits.push_back([this]() -> bool {
            this->CS2 = This()->FindModule<CSController>("CSController");
            this->CS2View = This()->FindModule<HookView>("HookView");
            this->CS2Time = This()->FindModule<TimeController>("TimeController");
            this->Hub = This()->FindModule<PlayerHub>("PlayerHub");
            this->CS2Con = This()->FindModule<HookConsole>("HookConsole");
            this->CS2Hashs = This()->FindModule<CS2Hash>("CS2Hash");

            this->CS2Paths = CS2Paths::Get();

            return true;
            });

        This()->postInits.push_back([this]()->bool {
            if (this->participateIt) {
                this->CS2->ParticipateItCSModules.push_back(this);
            }
            return true;
            });
    }

    void AsyncCommand(std::string&& cmd)const {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Game/Command"_hash);
        rp->str1 = std::move(cmd);
        This()->PublishAsync(std::move(msg));
    }
    void AsyncCommandNoReport(std::string&& cmd)const {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Game/Command/NoReport"_hash);
        rp->str1 = std::move(cmd);
        This()->PublishAsync(std::move(msg));
    }
};

class CSModuleBase :public MulNX::Module<CSModuleBase>, public CSModuleMixin<CSModuleBase> {};
template<typename T>
class CSModuleBaseT :public MulNX::Module<T>, public CSModuleMixin<T> {};