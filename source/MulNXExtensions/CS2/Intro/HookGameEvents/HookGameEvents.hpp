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
        auto GetInt(const CKV3MemberName& key) { return IVClass::Assume(this)->GetVFunc<int(const CKV3MemberName&)>(7)(key); }
        auto GetPlayerController(const CKV3MemberName& key) { return IVClass::Assume(this)->GetVFunc<CS2::CCSPlayerController * (const CKV3MemberName&)>(16)(key); }
        auto GetPlayerPawn(const CKV3MemberName& key) { return IVClass::Assume(this)->GetVFunc<CS2::C_CSPlayerPawn * (const CKV3MemberName&)>(17)(key); }
    };
}

class HookGameEvents final :public CSModuleBase {
    WrapHook hkCGameEventManager_FireEvent{};
    WrapHook hkPos_CGameEventManager_FireEvents_AcquiredLock{};
    bool Init()override;
};