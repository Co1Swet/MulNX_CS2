#pragma once
#include <Intro/Intros.hpp>

class BombSpotController :public CSModule<BombSpotController> {
    std::unique_ptr<MulNX::Hook> hkPos_Spot_WriteBombState = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnMaybeSetAllHUD = nullptr;
    MulReflBool(forceBombRedWhenSpecCT, true);
    void Menu();
    bool Init()override;
};