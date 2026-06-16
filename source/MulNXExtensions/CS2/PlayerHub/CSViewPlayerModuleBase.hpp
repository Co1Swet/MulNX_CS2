#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ICSViewPlayerModule {
public:
    virtual ~ICSViewPlayerModule() = default;
    virtual void HubPlayer(MulNX::UINode* node) {};
    virtual void HubTeam(MulNX::UINode* node) {};
    virtual void HubWindow(MulNX::UINode* node) {};
};

template <typename T>
class CSViewPlayerMixin :public ICSViewPlayerModule, public CSModuleMixin<T> {
    T* This() { return static_cast<T*>(this); }
public:
    CSViewPlayerMixin() {
        This()->delayInits->push_back([this]() -> bool {
            this->Hub->PlayerViewModules.push_back(This());
            return true;
            });
    }
};

class CSViewPlayerModuleBase :public MulNX::Module<CSViewPlayerModuleBase>, public CSViewPlayerMixin<CSViewPlayerModuleBase> {};
template<typename T>
class CSViewPlayerModuleBaseT :public MulNX::Module<T>, public CSViewPlayerMixin<T> {};