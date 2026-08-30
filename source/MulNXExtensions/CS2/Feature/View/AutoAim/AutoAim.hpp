#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>

class AutoAim final :public CSModuleBase, public CSViewControlMixin<AutoAim> {
    std::atomic<bool> enable = false;
    bool Init()override;
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;
};