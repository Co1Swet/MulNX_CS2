#pragma once
#include <Intro/Intros.hpp>

class PlayerHub final :public CSModuleBase {
    std::atomic<bool> ShowCompanionWindow = false;
    bool Init()override;
    void Window(MulNX::UICoordinator* uico);
};