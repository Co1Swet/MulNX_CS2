#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>

class AutoAim final :public CSModuleBase, public CSViewControlMixin<AutoAim> {
    class TargetPicker* pTargetPicker = nullptr;

    std::atomic<bool> enable = false;
    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;
};