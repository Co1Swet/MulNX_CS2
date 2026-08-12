#pragma once
#include <Intro/CSModuleBase.hpp>

class CS2Test final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkGetOBMode = nullptr;
    RetEditor OBRetEditor{};
    std::atomic<bool> forceReturn = false;
    std::atomic<int> forceReturnValue = 4;

    std::unique_ptr<MulNX::Hook>hkGetOBingPawn = nullptr;
    RetEditor OBingPawnRetEditor{};
    std::atomic<bool> forceReturnNullptr = false;

    void UI();
    bool Init()override;
};