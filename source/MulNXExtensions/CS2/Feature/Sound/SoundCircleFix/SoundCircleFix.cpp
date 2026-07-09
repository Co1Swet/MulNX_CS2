#include "SoundCircleFix.hpp"

bool SoundCircleFix::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto Pos_CallGetPawnUpdateCirclePos = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnUpdateCirclePos).Data();
        this->hkPos_CallGetPawnUpdateCirclePos = this->CreateHook("Pos_CallGetPawnUpdateCirclePos", Pos_CallGetPawnUpdateCirclePos, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load())return MulNX::Hook::Then::Continue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPos_CallGetPawnUpdateCirclePos.Attach();

        auto Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque).Data();
        Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque -= 5;
        this->hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque = this->CreateHook("Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque", Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load())return MulNX::Hook::Then::SkipAllAndContinue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, false, true).value();
        this->hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque.Attach();

        auto Pos_CallGetPawnMaybeOtherAsyncSoundEnque = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnMaybeOtherAsyncSoundEnque).Data();
        this->hkPos_CallGetPawnMaybeOtherAsyncSoundEnque = this->CreateHook("Pos_CallGetPawnMaybeOtherAsyncSoundEnque", Pos_CallGetPawnMaybeOtherAsyncSoundEnque, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load())return MulNX::Hook::Then::Continue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPos_CallGetPawnMaybeOtherAsyncSoundEnque.Attach();

        return;
        });

    this->runFlag1 = true;

    return true;
}