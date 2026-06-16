#pragma once
#include <MulNX/MulNX.hpp>

class UIDocker final :public MulNX::Module<UIDocker> {
    struct Padding {
        float top = 350;
        float bottom = 200;
        float left = 0;
        float right = 0;
    };
    Padding padding;
public:
    bool Init();
    void MainDraw(MulNX::UINode* node);
    void Window(MulNX::UINode* node);
};