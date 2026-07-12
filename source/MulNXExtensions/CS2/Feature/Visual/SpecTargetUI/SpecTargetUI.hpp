#pragma once
#include <Intro/CSModuleBase.hpp>

class SpecTargetUI final :public CSModuleBase {
    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
};