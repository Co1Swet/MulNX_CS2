#include "ReShowSpeaker.hpp"
#include <Buildup/TimeController/TimeController.hpp>

bool ReShowSpeaker::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::ifShowSpeaker).Data();
        this->hkPos_ifShowSpeaker = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->rax = 0;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_ifShowSpeaker, "Pos_ifShowSpeaker");

        this->pFuncGetVoiceStatus = (GetVoiceStatus_t)this->CS2->client.GetTextRegion()
            .FindRegion(MulNX::CS2::Signatures::Sound::GetVoiceStatus).Data();
        this->pFuncUpdateSpeakerStatus = (UpdateSpeakerStatus_t)this->CS2->client.GetTextRegion()
            .FindRegion(MulNX::CS2::Signatures::Sound::UpdateSpeakerStatus).Data();
        // if (*v8) {
        //     n2 = Msg("CVoiceStatus::UpdateSpeakerStatus: ent %d ss[%d] talking = %d\n", n0x3F, v7, v5);
        //     goto LABEL_72;
        // }
        });

    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...) {
        auto voiceStatus = this->pFuncGetVoiceStatus();
        if (!voiceStatus) return;
        auto old = this->lastUpdateTick.load(std::memory_order_acquire);
        if (old == 0) {
            this->lastUpdateTick.store(this->CS2Time->GetDemoTick(), std::memory_order_release);
            return;
        }
        auto now = this->CS2Time->GetDemoTick();
        if (std::abs(old - now) > 5) {
            for (uint32_t i = 0; i < 64; ++i) {
                this->pFuncUpdateSpeakerStatus(voiceStatus, i, -1, 0);
            }
        }
        this->lastUpdateTick.store(now, std::memory_order_release);
        });

    (*this)
        .SubscribeAsync<void>("ClearAllSpeakStatus");

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void ReShowSpeaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type){
    case "ClearAllSpeakStatus"_hash: {
        this->lastUpdateTick.store(1, std::memory_order_release);
        break;
    }
    default:
        break;
    }
}