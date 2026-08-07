#pragma once
#include <Intro/CSModuleBase.hpp>

class HookHealthAmmoCenter final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_CheckFor_HudSpecplayerRoot__visible = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_CheckFor_HUD__spectating_target = nullptr;

    std::atomic<bool>hideHudSpecplayerRoot = true;
    std::atomic<bool>show_Hud_HA__stroke = true;

    void Menu();
    bool Init()override;
};