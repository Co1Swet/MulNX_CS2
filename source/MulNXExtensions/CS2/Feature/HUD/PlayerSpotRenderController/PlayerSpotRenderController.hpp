#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXUtils/MemInsights/BitMaskEditor/BitMaskEditor.hpp>

class PlayerSpotRenderController :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_CmpToSetShow = nullptr;
    std::unique_ptr<MulNX::Hook>hkFunc_FinallyUpdatePlayerState = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = nullptr;

    std::atomic<bool>hideNumLabel = true;
    std::atomic<bool>forceTeammateDraw = true;
    std::atomic<bool>forceEnemyRed = true;
    void Menu();
    bool Init()override;
};