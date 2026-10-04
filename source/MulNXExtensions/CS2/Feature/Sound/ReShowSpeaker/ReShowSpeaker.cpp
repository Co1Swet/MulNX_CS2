#include "ReShowSpeaker.hpp"

bool ReShowSpeaker::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Sound::ifShowSpeaker).Data();
        this->hkPos_ifShowSpeaker = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->rax = 0;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_ifShowSpeaker, "Pos_ifShowSpeaker");

        this->pFuncGetVoiceStatus = (GetVoiceStatus_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Sound::GetVoiceStatus).Data();
        this->pFuncUpdateSpeakerStatus = (UpdateSpeakerStatus_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Sound::UpdateSpeakerStatus).Data();
        // if (*v8) {
        //     n2 = Msg("CVoiceStatus::UpdateSpeakerStatus: ent %d ss[%d] talking = %d\n", n0x3F, v7, v5);
        //     goto LABEL_72;
        // }
        });

    this->SubscribeSync("Hook/MainLoop/TickJumpDetected", [this](auto&&...) {
        auto voiceStatus = this->pFuncGetVoiceStatus();
        if (!voiceStatus) return;
        for (uint32_t i = 0; i < 64; ++i) {
            this->pFuncUpdateSpeakerStatus(voiceStatus, i, -1, 0);
        }
        });

    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...) {
        this->Update();
        });

    (*this)
        .SubscribeAsync<void>("ClearAllSpeakStatus");

    return true;
}

void ReShowSpeaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type){
    case "ClearAllSpeakStatus"_hash: {
        auto voiceStatus = this->pFuncGetVoiceStatus();
        if (!voiceStatus) return;
        for (uint32_t i = 0; i < 64; ++i) {
            this->pFuncUpdateSpeakerStatus(voiceStatus, i, -1, 0);
        }
        break;
    }
    default:
        break;
    }
}