#pragma once
#include <Intro/Intros.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>

class AutoDepth final :public CSModule<AutoDepth>, public CSViewControlMixin<AutoDepth> {
    bool Init()override;
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;
};