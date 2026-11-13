#pragma once
#include <Intro/Intros.hpp>

class HookHealthAmmoCenter final :public CSModule<HookHealthAmmoCenter> {
    std::unique_ptr<MulNX::Hook> hkPos_CheckFor_HudSpecplayerRoot__visible = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_CheckFor_HUD__spectating_target = nullptr;

    MulReflBool(hideHudSpecplayerRoot, true);
    MulReflBool(show_Hud_HA__stroke, true);

    void Menu();
    bool Init()override;
};