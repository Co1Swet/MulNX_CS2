#include "ReShowSpeaker.hpp"

bool ReShowSpeaker::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::ifShowSpeaker).Data();
        this->hkPos_ifShowSpeaker = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->rax = 0;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_ifShowSpeaker, "Pos_ifShowSpeaker");
        });

    return true;
}