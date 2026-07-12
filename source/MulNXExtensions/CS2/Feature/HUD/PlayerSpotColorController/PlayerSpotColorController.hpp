#pragma once
#include <Intro/CSModuleBase.hpp>

class PlayerSpotColorController :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetColor = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetTColor = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetCTColor = nullptr;

    std::atomic<bool>TColorMulti = true;
    std::atomic<bool>CTColorMulti = true;

    void Menu();
    bool Init()override;
};