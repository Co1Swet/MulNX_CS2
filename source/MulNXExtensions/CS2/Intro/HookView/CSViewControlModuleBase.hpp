#pragma once
#include "HookView.hpp"

class ICSViewControlModule {
public:
    MulNX::ModuleBase* pBaseFromViewControl = nullptr;
    virtual bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer) = 0;
};

template<typename T>
class CSViewControlMixin :public ICSViewControlModule {
    T* This() { return static_cast<T*>(this); }
protected:
    CSViewControlMixin() {
        this->pBaseFromViewControl = This();
        This()->preInits.push_back([this]() {
            auto pHookView = This()->FindModule<HookView>("HookView");
            pHookView->viewControlModules.push_back(This());
            return true;
            });
    }
};