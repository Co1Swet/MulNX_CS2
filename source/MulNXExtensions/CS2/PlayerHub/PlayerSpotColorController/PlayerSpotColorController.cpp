#include "PlayerSpotColorController.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool PlayerSpotColorController::Init() {

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改汇编分支进入着色
        auto Pos_CmpToSetColor = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetColor);
        this->hkPos_CmpToSetColor = MulNX::Hook::Create(Pos_CmpToSetColor.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->TColorMulti.load(std::memory_order_acquire) || this->CTColorMulti.load(std::memory_order_acquire))
                ctx->rbx = 0;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPos_CmpToSetColor->Attach();
        this->LogSucc(I18n("hook.attached", "Pos_CmpToSetColor"));

        // 让T显示五颜色
        auto Pos_CmpToSetTColor = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetTColor);
        this->hkPos_CmpToSetTColor = MulNX::Hook::Create(Pos_CmpToSetTColor.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->TColorMulti.load(std::memory_order_acquire))
                ctx->rax = 2;
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, false, true).value();
        this->hkPos_CmpToSetTColor->Attach();
        this->LogSucc(I18n("hook.attached", "Pos_CmpToSetTColor"));

        // 让CT显示五颜色
        auto Pos_CmpToSetCTColor = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CmpToSetCTColor);
        this->hkPos_CmpToSetCTColor = MulNX::Hook::Create(Pos_CmpToSetCTColor.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->CTColorMulti.load(std::memory_order_acquire))
                ctx->rax = 3;
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, false, true).value();
        this->hkPos_CmpToSetCTColor->Attach();
        this->LogSucc(I18n("hook.attached", "Pos_CmpToSetCTColor"));

        });

    return true;
}