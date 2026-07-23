#include "SoundCircleFix.hpp"

bool SoundCircleFix::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto Pos_CallGetPawnUpdateCirclePos = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnUpdateCirclePos).Data();
        this->hkPos_CallGetPawnUpdateCirclePos = MulNX::Hook::Create(Pos_CallGetPawnUpdateCirclePos, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load())return MulNX::Hook::Then::Continue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkPos_CallGetPawnUpdateCirclePos, "Pos_CallGetPawnUpdateCirclePos");

        auto Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque).Data();
        Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque -= 5;
        this->hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque = MulNX::Hook::Create(Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load())return MulNX::Hook::Then::SkipAllAndContinue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, false, true).value();
        this->RegisterAttachHook(this->hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque, "Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque");

        auto Pos_CallGetPawnMaybeOtherAsyncSoundEnque = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Sound::Pos_CallGetPawnMaybeOtherAsyncSoundEnque).Data();
        this->hkPos_CallGetPawnMaybeOtherAsyncSoundEnque = MulNX::Hook::Create(Pos_CallGetPawnMaybeOtherAsyncSoundEnque, [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load())return MulNX::Hook::Then::Continue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (pOBing)ctx->rax = (uint64_t)pOBing;
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkPos_CallGetPawnMaybeOtherAsyncSoundEnque, "Pos_CallGetPawnMaybeOtherAsyncSoundEnque");

        // auto testpos = this->CS2->client.GetBaseAddress() + 0xA100FA;
        // static auto testhk = MulNX::Hook::Create((uint8_t*)testpos, [this](MulNX::Hook*, RegContext* ctx) {
        //     auto pOBing = this->CS2->client.TryGetObservingPawn();
        //     if (!pOBing) return MulNX::Hook::Then::Continue;

        //     using LocalEnqueueFn = __int64(__fastcall*)(uintptr_t, int, __int64, char);
        //     auto LocalEnqueue = reinterpret_cast<LocalEnqueueFn>(
        //         this->CS2->client.GetBaseAddress() + 0xE33370);
        //     LocalEnqueue(reinterpret_cast<uintptr_t>(pOBing), 0x44C, 0x2F, 1);

        //     return MulNX::Hook::Then::Continue;
        //     }, true).value();
        // testhk->Attach();

        });

    this->runFlag1 = true;

    return true;
}