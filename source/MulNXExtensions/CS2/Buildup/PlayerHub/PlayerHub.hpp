#pragma once
#include <Intro/CSModuleBase.hpp>

class PlayerHub final :public CSModuleBase {
    std::atomic<bool> ShowCompanionWindow = false;
    bool Init()override;
    bool Window(MulNX::UICoordinator* uico);
};