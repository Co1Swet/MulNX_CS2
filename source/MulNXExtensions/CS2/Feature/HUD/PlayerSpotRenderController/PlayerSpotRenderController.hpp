#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXUtils/MemInsights/BitMaskEditor/BitMaskEditor.hpp>

class PlayerSpotRenderController :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_CmpToSetShow = nullptr;
    std::unique_ptr<MulNX::Hook>hkFunc_FinallyUpdatePlayerState = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = nullptr;

    MulReflBool(hideNumLabel, true);
    MulReflBool(transSpecLabel, true);
    MulReflBool(forceTeammateDraw, true);
    MulReflBool(forceEnemyRed, true);
    void Menu();
    bool Init()override;
};