#pragma once
#include <MUlNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class PlayerSpotController :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_CmpToSetShow = nullptr;
    std::unique_ptr<MulNX::Hook>hkFunc_FinallyUpdatePlayerState = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteMaybeEnumToChangeRadarPlayerDraw = nullptr;
public:
    bool Init()override;
};