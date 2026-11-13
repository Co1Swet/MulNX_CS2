#pragma once
#include <Intro/Intros.hpp>

class HookSpotStyle final :public CSModule<HookSpotStyle> {
    std::unique_ptr<MulNX::Hook> hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = nullptr;
    std::unique_ptr<MulNX::Hook> hkFunc_FinallyUpdatePlayerState = nullptr;

    MulReflBool(hideNumLabel, true);
    MulReflBool(transSpecLabel, true);
    MulReflBool(repairEnenyStyle, true);

    void Menu();
    bool Init()override;
};