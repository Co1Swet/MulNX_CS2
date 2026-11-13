#pragma once
#include <Intro/Intros.hpp>
#include <MulNXUtils/MemInsights/BitMaskEditor/BitMaskEditor.hpp>

class HookSpotVisible :public CSModule<HookSpotVisible> {
    std::unique_ptr<MulNX::Hook> hkPos_Spot_CmpToSetShow = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_SetAllVisible = nullptr;

    MulReflBool(forceTeammateDraw, true);
    MulReflBool(noAllVisible, true);

    void Menu();
    bool Init()override;
};