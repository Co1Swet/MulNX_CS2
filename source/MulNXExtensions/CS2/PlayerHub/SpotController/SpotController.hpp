#pragma once
#include <MUlNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

class SpotController :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_Spot_CmpToSetShow = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_Spot_WriteBombState = nullptr;
public:
    bool Init()override;
};