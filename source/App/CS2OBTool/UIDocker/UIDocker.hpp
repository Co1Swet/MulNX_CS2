#pragma once
#include <MulNX/MulNX.hpp>

class UIDocker final :public MulNX::Module<UIDocker> {
    bool Init();
    void MainDraw(MulNX::UICoordinator* uico);
};