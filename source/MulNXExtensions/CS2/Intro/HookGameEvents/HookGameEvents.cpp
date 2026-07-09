#include "HookGameEvents.hpp"

// typedef CS2::CGameEvent* (*CGameEventManager_CreateEvent_t)(void* This, const char* name, bool bForce /*= false*/, int* pCookie /*= NULL*/); //:006
// typedef bool (*CGameEventManager_FireEvent_t)(void* This, CS2::CGameEvent* event, bool bDontBroadcast /*= false*/); //:007
// typedef bool (*CGameEventManager_FireEventClientSide_t)(void* This, CS2::CGameEvent* event); //:008
// typedef void (*CGameEventManager_FreeEvent_t)(void* This, CS2::CGameEvent* event); //:010

bool HookGameEvents::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // this->hkCGameEventManager_FireEvent=this->CreateHook("CGameEventManager_FireEvent", (uint8_t*)vtable[7], [this](MulNX::Hook* hk, RegContext* ctx) {
        //     auto event = std::bit_cast<CS2::CGameEvent*>(ctx->rdx);
        //     //this->PublishSync("Hook/CGameEventManager_FireEvent"_hash, event);
        //     return MulNX::Hook::Then::Continue;
        //     }).value();
        // this->hkCGameEventManager_FireEvent.Attach();

        auto Pos_CGameEventManager_FireEvents_AcquiredLock = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Utils::Pos_CGameEventManager_FireEvents_AcquiredLock).Data();
        this->hkPos_CGameEventManager_FireEvents_AcquiredLock = this->CreateHook("Pos_CGameEventManager_FireEvents_AcquiredLock where r14 is *event", Pos_CGameEventManager_FireEvents_AcquiredLock, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto event = reinterpret_cast<CS2::CGameEvent*>(ctx->r14);
            auto msg = MulNX::Message("Hook/FireEventClientSide"_hash);
            auto&& [pEvent] = msg.Access<CS2::CGameEvent*>();
            pEvent = event;
            this->PublishSync(msg);
#if _DEBUG
            auto name = event->GetEventName();
#endif
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->hkPos_CGameEventManager_FireEvents_AcquiredLock.Attach();
        });

    return true;
}