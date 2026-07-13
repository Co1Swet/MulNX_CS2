#include "HookGameEvents.hpp"

// typedef CS2::CGameEvent* (*CGameEventManager_CreateEvent_t)(void* This, const char* name, bool bForce /*= false*/, int* pCookie /*= NULL*/); //:006
// typedef bool (*CGameEventManager_FireEvent_t)(void* This, CS2::CGameEvent* event, bool bDontBroadcast /*= false*/); //:007
// typedef bool (*CGameEventManager_FireEventClientSide_t)(void* This, CS2::CGameEvent* event); //:008
// typedef void (*CGameEventManager_FreeEvent_t)(void* This, CS2::CGameEvent* event); //:010

enum class CS2GameEventID :int {
    player_hurt = 16,
    player_death = 53,
    player_footstep = 54,
    weapon_fire = 160,
    player_blind = 203
};

consteval uint64_t PraseGameEvent(const CS2GameEventID& event) {
    switch (event) {
    case CS2GameEventID::player_hurt:return "Hook/FireEventClientSide/player_hurt"_hash;
    case CS2GameEventID::player_death:return "Hook/FireEventClientSide/player_death"_hash;
    case CS2GameEventID::player_footstep:return "Hook/FireEventClientSide/player_footstep"_hash;
    case CS2GameEventID::weapon_fire:return "Hook/FireEventClientSide/weapon_fire"_hash;
    case CS2GameEventID::player_blind:return "Hook/FireEventClientSide/player_blind"_hash;
    }
}

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
            
            auto id = event->GetID();
            uint64_t hash;
            switch (static_cast<CS2GameEventID>(id)) {
            case CS2GameEventID::player_hurt:hash = PraseGameEvent(CS2GameEventID::player_hurt);break;
            case CS2GameEventID::player_death:hash = PraseGameEvent(CS2GameEventID::player_death);break;
            default: return MulNX::Hook::Then::Continue;
            }
            MulNX::Message msg(hash);
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