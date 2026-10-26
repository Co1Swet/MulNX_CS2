#pragma once
#include <Intro/CSModuleBase.hpp>

class HookSpotColor :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetColor = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetTColor = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_CmpToSetCTColor = nullptr;

    MulReflBool(TColorMulti, true);
    MulReflBool(CTColorMulti, true);

    void Menu();
    bool Init()override;
};