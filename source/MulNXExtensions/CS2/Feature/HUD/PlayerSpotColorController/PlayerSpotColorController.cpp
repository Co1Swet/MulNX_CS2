#include "PlayerSpotColorController.hpp"
#include <MulNX/Base/UI/UI.hpp>

void PlayerSpotColorController::Menu() {
    MulNX::UI::Checkbox("让T方显示五种颜色", this->TColorMulti);
    MulNX::UI::Checkbox("让CT方显示五种颜色", this->CTColorMulti);
}

bool PlayerSpotColorController::Init() {

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改汇编分支进入着色
        auto Pos_CmpToSetColor = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetColor);
        this->hkPos_CmpToSetColor = MulNX::Hook::Create(Pos_CmpToSetColor.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->TColorMulti.load(std::memory_order_acquire) || this->CTColorMulti.load(std::memory_order_acquire))
                ctx->rbx = 0;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_CmpToSetColor, "Pos_CmpToSetColor");

        // 让T显示五颜色
        auto Pos_CmpToSetTColor = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetTColor);
        this->hkPos_CmpToSetTColor = MulNX::Hook::Create(Pos_CmpToSetTColor.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->TColorMulti.load(std::memory_order_acquire))
                ctx->rax = 2;
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, true, true).value();
        this->RegisterAttachHook(this->hkPos_CmpToSetTColor, "Pos_CmpToSetTColor");

        // 让CT显示五颜色
        auto Pos_CmpToSetCTColor = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetCTColor);
        this->hkPos_CmpToSetCTColor = MulNX::Hook::Create(Pos_CmpToSetCTColor.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->CTColorMulti.load(std::memory_order_acquire))
                ctx->rax = 3;
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, true, true).value();
        this->RegisterAttachHook(this->hkPos_CmpToSetCTColor, "Pos_CmpToSetCTColor");
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->Menu();});

    return true;
}