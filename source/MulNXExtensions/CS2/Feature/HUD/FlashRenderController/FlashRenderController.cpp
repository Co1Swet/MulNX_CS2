#include "FlashRenderController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>

void FlashRenderController::Menu() {
    ImGui::SliderFloat("闪光绘制最大强度", this->r_spectator_flashbang_opacity, 0.0f, 1.0f);
    MulNX::UI::Checkbox("是否在HUD上执行闪光绘制", this->rendFlashUpHUD);
    MulNX::UI::Checkbox("是否在HUD下执行闪光绘制", this->rendFlashDownHUD);
}

bool FlashRenderController::Init() {
    this->SubscribeSync("Hook/Source2Client002::Inited", [this](MulNX::Message& msg) {
        auto up = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Flash::PosCallCmpDrawFlashUpHUD).Data();
        this->hkDrawUp = MulNX::Hook::Create(up + 11, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->rendFlashUpHUD.load(std::memory_order_relaxed)) {
                ctx->rax = 0ULL;
            }
            else {
                ctx->rax = 1ULL;
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkDrawUp, "PosCallCmpDrawFlashUpHUD");

        auto down = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Flash::PosCallCmpDrawFlashDownHUD).Data();
        down -= 5;
        this->hkDrawDown = MulNX::Hook::Create(down, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->rendFlashDownHUD.load(std::memory_order_relaxed)) {
                ctx->rax = 1ULL;
            }
            else {
                ctx->rax = 0ULL;
            }
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, false, true).value();
        this->RegisterAttachHook(this->hkDrawDown, "PosCallCmpDrawFlashDownHUD");

        this->r_spectator_flashbang_opacity = this->CS2Con->GetCVarByName("r_spectator_flashbang_opacity")->GetPtr<float>();
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->Menu();});

    return true;
}