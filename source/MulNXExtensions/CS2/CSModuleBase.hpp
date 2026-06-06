#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CS2/CSClasses/Consoles.hpp>

class CSController;
class PlayerHub;
class ViewController;
class TimeController;
class HookConsole;

using Steam64UID = uint64_t;

template <typename T>
class CSModuleMixin {
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
        mod->delayInits.push_back([this, mod]() -> bool {
            this->CS2 = mod->GetCore()->ModuleManager()->FindModule<CSController>("CSController");
            this->CS2View = mod->GetCore()->ModuleManager()->FindModule<ViewController>("ViewController");
            this->CS2Time = mod->GetCore()->ModuleManager()->FindModule<TimeController>("TimeController");
            this->Hub = mod->GetCore()->ModuleManager()->FindModule<PlayerHub>("PlayerHub");
            this->CS2Con = mod->GetCore()->ModuleManager()->FindModule<HookConsole>("HookConsole");
            return true;
        });
    }
};

class CSModuleBase :public MulNX::ModuleBase, public CSModuleMixin<CSModuleBase> {};