#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ICSViewPlayerModule {
public:
    virtual ~ICSViewPlayerModule() = default;
    virtual void Player(MulNX::UINode* node) = 0;
    virtual void Team(MulNX::UINode* node) = 0;
};

template <typename T>
class CSViewPlayerMixin :public ICSViewPlayerModule, public CSModuleMixin<T> {
public:
    CSViewPlayerMixin() {
        static_assert(MulNX::Module<T>, "T must be a MulNX Module");
        auto* mod = static_cast<MulNX::ModuleBase*>(static_cast<T*>(this));
        mod->delayInits.push_back([this, mod]() -> bool {
            this->Hub->PlayerViewModules.push_back(this);
            return true;
            });
    }
};

class CSViewPlayerModuleBase :public MulNX::ModuleBase, public CSViewPlayerMixin<CSViewPlayerModuleBase> {};