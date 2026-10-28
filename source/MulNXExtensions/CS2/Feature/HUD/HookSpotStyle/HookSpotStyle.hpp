#pragma once
#include <Intro/CSModuleBase.hpp>

class HookSpotStyle final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = nullptr;
    std::unique_ptr<MulNX::Hook> hkFunc_FinallyUpdatePlayerState = nullptr;

    MulReflBool(hideNumLabel, true);
    MulReflBool(transSpecLabel, true);
    MulReflBool(repairEnenyStyle, true);

    void Menu();
    bool Init()override;
};