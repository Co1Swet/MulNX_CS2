#pragma once
#include <Intro/Intros.hpp>

class CS2Test final :public CSModule<CS2Test> {
    void UI();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};