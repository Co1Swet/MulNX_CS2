#pragma once
#include <Intro/CSModuleBase.hpp>

class BombSpotController :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_Spot_WriteBombState = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnMaybeSetAllHUD = nullptr;
    std::atomic<bool>forceBombRedWhenSpecCT = true;
    void Menu();
    bool Init()override;
};