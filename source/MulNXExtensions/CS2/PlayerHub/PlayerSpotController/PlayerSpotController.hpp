#pragma once
#include <MUlNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class PlayerSpotController :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_CmpToSetShow = nullptr;
    std::unique_ptr<MulNX::Hook>hkFunc_FinallyUpdatePlayerState = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = nullptr;

    std::atomic<bool>hideNumLabel = true;
    std::atomic<bool>forceTeammateDraw = true;
    std::atomic<bool>forceEnemyRed = true;
    void HubWindow(MulNX::UINode* node)override;
public:
    bool Init()override;
};