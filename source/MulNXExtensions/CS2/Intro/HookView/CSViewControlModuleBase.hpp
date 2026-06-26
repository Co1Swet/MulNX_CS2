#pragma once
#include "HookView.hpp"
#include <Intro/CSClasses/tree/tree.hpp>

class ICSViewControlModule {
public:
    virtual bool HandleUpdate(CS2::CViewSetup* viewSetup, const int& num) = 0;
    virtual MulNX::ModuleBase* ViewControlToBase() = 0;
};

template<typename T>
class CSViewControlMixin :public ICSViewControlModule {
    T* This() { return static_cast<T*>(this); }
protected:
    CSViewControlMixin() {
        This()->delayInits->push_back([this]() {
            auto pHookView = This()->FindModule<HookView>("HookView");
            pHookView->viewControlModules.push_back(This());
            return true;
            });
    }
public:
    MulNX::ModuleBase* ViewControlToBase()override { return static_cast<MulNX::ModuleBase*>(This()); }
};