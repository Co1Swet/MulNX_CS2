#include "HookGSI.hpp"
#include <Intro/HookView/HookView.hpp>

bool HookGSI::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::GSI::Pos_GSI_ID_ForSpecTarget_call_GetOBingPawn).Data();
        this->hkPos_GSI_ForSpecTarget_call_GetOBingPawn = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->CS2View->GetCameraLeavePlayerState()) {
                ctx->rax = 0;
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_GSI_ForSpecTarget_call_GetOBingPawn, "Pos_GSI_ForSpecTarget_call_GetOBingPawn");
        });

    return true;
}