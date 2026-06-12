#pragma once
#include <MUlNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class BombSpotController :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteBombState = nullptr;
    void HubWindow(MulNX::UINode* node)override;
public:
    bool Init()override;
};