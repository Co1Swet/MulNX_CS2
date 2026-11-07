#pragma once
#include <Intro/Intros.hpp>

class CS2Test final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkTest = nullptr;

    void UI();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};