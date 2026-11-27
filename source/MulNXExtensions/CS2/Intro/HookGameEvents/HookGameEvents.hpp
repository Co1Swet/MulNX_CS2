#pragma once
#include <Intro/CSModuleBase.hpp>

namespace CS2 {
    struct CKV3MemberName {
        uint32_t hash;
        int index;
        const char* str;
    };
    class CGameEvent {
        CGameEvent() = delete;
        virtual ~CGameEvent() = 0;
    public:
        virtual const char* GetEventName() = 0;
        virtual int GetID() = 0;
        virtual void unk003() = 0;
        virtual void unk004() = 0;
        virtual void unk005() = 0;
        virtual void unk006() = 0;
        virtual int GetInt(const CKV3MemberName&) = 0;
        virtual uint64_t GetUint64(const CKV3MemberName& keySymbol) = 0;
        virtual void unk009() = 0;
        virtual const char* GetString(const CKV3MemberName&) = 0;
        virtual void unk011() = 0;
        virtual void unk012() = 0;
        virtual void unk013() = 0;
        virtual void unk014() = 0;
        virtual void unk015() = 0;

        virtual CS2::CCSPlayerController* GetPlayerController(const CKV3MemberName& key) = 0;
        virtual CS2::C_CSPlayerPawn* GetPlayerPawn(const CKV3MemberName& key) = 0;
    };
}

class HookGameEvents final :public CSModule<HookGameEvents> {
    std::unique_ptr<MulNX::Hook> hkCGameEventManager_FireEvent{};
    std::unique_ptr<MulNX::Hook> hkPos_CGameEventManager_FireEvents_AcquiredLock{};
    bool Init()override;
};