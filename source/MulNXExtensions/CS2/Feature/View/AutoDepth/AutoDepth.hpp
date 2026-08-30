#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>

class AutoDepth final :public CSModuleBase, public CSViewControlMixin<AutoDepth> {
    bool Init()override;
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;
};