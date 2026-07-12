#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class BombSpotController :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteBombState = nullptr;
    WrapHook hkPos_CallGetPawnMaybeSetAllHUD{};
    void Menu();
    bool Init()override;
};