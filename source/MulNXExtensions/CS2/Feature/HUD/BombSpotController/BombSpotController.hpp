#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class BombSpotController :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteBombState = nullptr;
    void Menu(MulNX::UINode* node);
public:
    bool Init()override;
};