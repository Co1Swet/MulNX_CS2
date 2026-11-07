#pragma once
#include <Intro/Intros.hpp>

class SpecTargetUI final :public CSModuleBase {
    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
};