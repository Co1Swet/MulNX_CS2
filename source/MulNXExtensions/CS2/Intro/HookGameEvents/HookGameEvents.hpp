#pragma once
#include <Intro/CSModuleBase.hpp>

namespace CS2 {
    struct CKV3MemberName {
        uint32_t hash;
        int index;
        const char* str;
    };
    class CGameEvent {        
    public:
        auto GetEventName() { return IVClass::Assume(this)->GetVFunc<const char* ()>(1)(); }
        auto GetPlayerController(const CKV3MemberName& key) { return IVClass::Assume(this)->GetVFunc<CS2::CCSPlayerController * (const CKV3MemberName&)>(16)(key); }
    };
}

class HookGameEvents final :public CSModuleBase {
    WrapHook hkCGameEventManager_FireEvent{};
    WrapHook hkCGameEventManager_FireEventClientSide{};
    bool Init()override;
};