#pragma once
#include <MulNX/MulNX.hpp>

class UIDocker final :public MulNX::Module<UIDocker> {
public:
    bool Init();
    void MainDraw(MulNX::UINode* node);
};