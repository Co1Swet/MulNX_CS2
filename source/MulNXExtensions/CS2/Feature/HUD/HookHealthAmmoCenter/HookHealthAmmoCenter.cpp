#include "HookHealthAmmoCenter.hpp"

void HookHealthAmmoCenter::Menu() {
    MulNX::UI::Checkbox("隐藏下方观战提示", this->hideHudSpecplayerRoot);
    MulNX::UI::Checkbox("展示下方一根线", this->show_Hud_HA__stroke);
}

bool HookHealthAmmoCenter::Init() {
    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {
        this->Menu();
        });

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto t1 = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Hud::Pos_CheckFor_HudSpecplayerRoot__visible).Data();
        this->hkPos_CheckFor_HudSpecplayerRoot__visible = MulNX::Hook::Create(t1, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->rbx = !this->hideHudSpecplayerRoot;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_CheckFor_HudSpecplayerRoot__visible, "Pos_CheckFor_HudSpecplayerRoot__visible where rbx is bHudSpecplayerRootIsVisible");

        auto t2 = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Hud::Pos_CheckFor_HUD__spectating_target).Data();
        this->hkPos_CheckFor_HUD__spectating_target = MulNX::Hook::Create(t2, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->rdx = !this->show_Hud_HA__stroke;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_CheckFor_HUD__spectating_target, "Pos_CheckFor_HUD__spectating_target where rdx is bNeedSet_HUD__spectating_target (Hide .hud-HA__stroke)");

        });

    return true;
}