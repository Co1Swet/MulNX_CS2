#include "HookGameEvents.hpp"
#include <MulNXThirdParty/hlae/binutils.h>

// typedef CS2::CGameEvent* (*CGameEventManager_CreateEvent_t)(void* This, const char* name, bool bForce /*= false*/, int* pCookie /*= NULL*/); //:006
// typedef bool (*CGameEventManager_FireEvent_t)(void* This, CS2::CGameEvent* event, bool bDontBroadcast /*= false*/); //:007
// typedef bool (*CGameEventManager_FireEventClientSide_t)(void* This, CS2::CGameEvent* event); //:008
// typedef void (*CGameEventManager_FreeEvent_t)(void* This, CS2::CGameEvent* event); //:010

bool HookGameEvents::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        size_t pVtable = Afx::BinUtils::FindClassVtable(this->CS2->client.hModule, ".?AVCGameEventManager@@", 0, 0);
        if (!pVtable)MulNX::ErrorTerminate("HookGameEvents::Init() failed to find vtable for CGameEventManager");
        void** vtable = (void**)pVtable;
        // this->hkCGameEventManager_FireEvent=this->CreateHook("CGameEventManager_FireEvent", (uint8_t*)vtable[7], [this](MulNX::Hook* hk, RegContext* ctx) {
        //     auto event = std::bit_cast<CS2::CGameEvent*>(ctx->rdx);
        //     //this->PublishSync("Hook/CGameEventManager_FireEvent"_hash, event);
        //     return MulNX::Hook::Then::Continue;
        //     }).value();
        // this->hkCGameEventManager_FireEvent.Attach();

        this->hkCGameEventManager_FireEventClientSide = this->CreateHook("CGameEventManager_FireEventClientSide", (uint8_t*)vtable[8], [this](MulNX::Hook* hk, RegContext* ctx) {
            auto event = std::bit_cast<CS2::CGameEvent*>(ctx->rdx);
            auto msg = MulNX::Message("Hook/FireEventClientSide"_hash);
            auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();
            pEvent = event;
            this->PublishSync(msg);
#if _DEBUG
            auto name = event->GetEventName();
#endif
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkCGameEventManager_FireEventClientSide.Attach();
        });

    return true;
}