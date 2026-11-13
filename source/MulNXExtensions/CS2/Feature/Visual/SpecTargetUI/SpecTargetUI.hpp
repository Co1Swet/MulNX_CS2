#pragma once
#include <Intro/Intros.hpp>

class SpecTargetUI final :public CSModule<SpecTargetUI> {
    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
};