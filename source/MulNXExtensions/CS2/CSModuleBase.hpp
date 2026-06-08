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
    virtual void OnItEntity(CS2::C_BaseEntity*) {};
    virtual void OnItPlayer(CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*) {};
    virtual void OnItEnd() {};
};

template <typename T>
class CSModuleMixin: public ICSModule {
public:
    CSController* CS2 = nullptr;
    ViewController* CS2View = nullptr;
    TimeController* CS2Time = nullptr;
    PlayerHub* Hub = nullptr;
    HookConsole* CS2Con = nullptr;
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