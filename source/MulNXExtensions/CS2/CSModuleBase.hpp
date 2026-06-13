#pragma once
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

class PlayerHub;
class ViewController;
class TimeController;
class HookConsole;

using Steam64UID = uint64_t;

class ICSModule {
protected:
    static constexpr bool ParticipateIt = false;
public:
    ~ICSModule() = default;

    virtual void OnItBegin() {};
    virtual void OnItEntity(int index, CS2::C_BaseEntity*) {};
    virtual void OnItPlayer(int index, CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*) {};
    virtual void OnItEnd() {};
};

template <typename T>
class CSModuleMixin : public ICSModule {
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
public:
    CSController* CS2 = nullptr;
    ViewController* CS2View = nullptr;
    TimeController* CS2Time = nullptr;
    PlayerHub* Hub = nullptr;
    HookConsole* CS2Con = nullptr;
    CS2Paths* CS2Paths = nullptr;
protected:
    CSModuleMixin() {
        static_assert(MulNX::Module<T>, "T must be a MulNX Module");
        auto* mod = static_cast<MulNX::ModuleBase*>(static_cast<T*>(this));
        mod->delayInits->push_back([this, mod]() -> bool {
            this->CS2 = mod->GetCore()->ModuleManager()->FindModule<CSController>("CSController");
            this->CS2View = mod->GetCore()->ModuleManager()->FindModule<ViewController>("ViewController");
            this->CS2Time = mod->GetCore()->ModuleManager()->FindModule<TimeController>("TimeController");
            this->Hub = mod->GetCore()->ModuleManager()->FindModule<PlayerHub>("PlayerHub");
            this->CS2Con = mod->GetCore()->ModuleManager()->FindModule<HookConsole>("HookConsole");

            this->CS2Paths = CS2Paths->Get();

            if constexpr (T::ParticipateIt) {
                this->CS2->ParticipateItCSModules.push_back(this);
            }

            return true;
            });
    }
};

class CSModuleBase :public MulNX::ModuleBase, public CSModuleMixin<CSModuleBase> {};
template<typename T>
class CSModuleBaseT :public MulNX::ModuleBase, public CSModuleMixin<T> {};