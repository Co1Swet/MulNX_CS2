#pragma once
#include <Intro/CSModuleBase.hpp>

class CS2Test final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkTest = nullptr;

    void UI();
    bool Init()override;
};