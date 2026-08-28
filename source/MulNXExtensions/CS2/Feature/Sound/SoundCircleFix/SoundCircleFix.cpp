#include "SoundCircleFix.hpp"

bool SoundCircleFix::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto Pos_CallGetPawnUpdateCirclePos = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnUpdateCirclePos).Data();
        this->hkPos_CallGetPawnUpdateCirclePos = MulNX::Hook::Create(Pos_CallGetPawnUpdateCirclePos, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->enable.load())return MulNX::Hook::Then::Continue;
            auto pOBing = this->CS2Entitys->TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_CallGetPawnUpdateCirclePos, "Pos_CallGetPawnUpdateCirclePos");

        auto Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque).Data();
        Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque -= 5;
        this->hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque = MulNX::Hook::Create(Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->enable.load())return MulNX::Hook::Then::SkipAllAndContinue;
            auto pOBing = this->CS2Entitys->TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, true, true).value();
        this->RegisterAttachHook(this->hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque, "Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque");

        auto Pos_CallGetPawnMaybeOtherAsyncSoundEnque = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnMaybeOtherAsyncSoundEnque).Data();
        this->hkPos_CallGetPawnMaybeOtherAsyncSoundEnque = MulNX::Hook::Create(Pos_CallGetPawnMaybeOtherAsyncSoundEnque, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->enable.load())return MulNX::Hook::Then::Continue;
            try {
                auto pOBing = this->CS2Entitys->TryGetObservingPawn();
                if (!pOBing)return MulNX::Hook::Then::Continue;
                ctx->rax = std::bit_cast<uint64_t>(pOBing);
            }
            catch (std::exception& e) {
                this->LogError(e.what());
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_CallGetPawnMaybeOtherAsyncSoundEnque, "Pos_CallGetPawnMaybeOtherAsyncSoundEnque");
        });

    return true;
}