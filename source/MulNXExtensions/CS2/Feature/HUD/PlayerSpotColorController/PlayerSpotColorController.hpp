#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class PlayerSpotColorController :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetColor = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetTColor = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetCTColor = nullptr;

    std::atomic<bool>TColorMulti = true;
    std::atomic<bool>CTColorMulti = true;
public:
    bool Init()override;
    void IDraw(MulNX::UINode* node);
};