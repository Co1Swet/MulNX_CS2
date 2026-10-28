#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXUtils/MemInsights/BitMaskEditor/BitMaskEditor.hpp>

class HookSpotVisible :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_Spot_CmpToSetShow = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_SetAllVisible = nullptr;

    MulReflBool(forceTeammateDraw, true);
    MulReflBool(noAllVisible, true);

    void Menu();
    bool Init()override;
};