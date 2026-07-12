#pragma once
#include <Intro/CSModuleBase.hpp>

class BombSpotController :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteBombState = nullptr;
    WrapHook hkPos_CallGetPawnMaybeSetAllHUD{};
    void Menu();
    bool Init()override;
};