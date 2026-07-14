#include "AntiVoiceBan.hpp"

bool AntiVoiceBan::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto) {
        auto target = this->CS2->client.GetTextRegion()
            .FindRegion(MulNX::CS2::Signatures::Sound::Func_ProcessVoiceBan).FindFuncStart().Data();

        this->hkProcessVoiceBan = this->CreateHook("ProcessVoiceBan", target, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->r8 = 0;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkProcessVoiceBan.Attach();
        });

    return true;
}