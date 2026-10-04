#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Game/StaticWeapons/StaticWeaponBase.hpp>
#include <Support/DamageRecorder/DamageRecorder.hpp>
#include <Support/ClientPanorama/ClientPanorama.hpp>

namespace CS2 {
    class CDamageRecord {
    public:
        uint8_t pad[cs2_dumper::schemas::client_dll::CDamageRecord::m_PlayerDamager];
        CS2::CHandle<CS2::C_CSPlayerPawn> m_PlayerDamager{}; // CHandle<C_CSPlayerPawn>
        CS2::CHandle<CS2::C_CSPlayerPawn> m_PlayerRecipient{}; // CHandle<C_CSPlayerPawn>
        CS2::CHandle<CS2::CCSPlayerController> m_hPlayerControllerDamager{}; // CHandle<CCSPlayerController>
        CS2::CHandle<CS2::CCSPlayerController> m_hPlayerControllerRecipient{}; // CHandle<CCSPlayerController>
        void* m_szPlayerDamagerName{}; // CUtlString
        void* m_szPlayerRecipientName{}; // CUtlString
        uint64_t m_DamagerXuid{}; // uint64
        uint64_t m_RecipientXuid{}; // uint64
        float m_flBulletsDamage{}; // float32
        float m_flDamage{}; // float32
        float m_flActualHealthRemoved{}; // float32
        int32_t m_iNumHits{}; // int32
        int32_t m_iLastBulletUpdate{}; // int32
        bool m_bIsOtherEnemy{}; // bool
        EKillTypes_t m_killType = EKillTypes_t::KILL_NONE; // EKillTypes_t
    };

    class CCSPlayerController_DamageServices {
        CCSPlayerController_DamageServices() = delete;
    public:
        uint8_t pad[cs2_dumper::schemas::client_dll::CCSPlayerController_DamageServices::m_nSendUpdate];
        int32_t m_nSendUpdate;
        uint8_t pad1[0x48 - 0x40 - sizeof(int32_t)];
        CS2::C_UtlVectorEmbeddedNetworkVar<CDamageRecord> m_DamageList;
    };
}

class HookDamageReport final :public CSModuleBase {
    ClientPanorama* pClientPanorama = nullptr;
    DamageRecorder* pDamageRecorder = nullptr;
    bool needUpdate = false;
    std::unique_ptr<MulNX::Hook> hkPos_Check_m_nSendUpdate = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_GettedController = nullptr;
    std::unique_ptr<MulNX::Hook> hkFunc_UpdateDamageReport = nullptr;
    using UpdateDamageReport_t = uint64_t(*)(uint64_t);
    UpdateDamageReport_t pFunc_UpdateDamageReport = nullptr;
    using DispatchClearAllPostRoundDamageReportPanels_t = void(*)(uint64_t);
    DispatchClearAllPostRoundDamageReportPanels_t pFunc_DispatchClearAllPostRoundDamageReportPanels = nullptr;

    std::atomic<bool> enable = true;

    void Menu();
    bool Init()override;
    void RefreshPool(CS2::CCSPlayerController* pObservedController, CS2::CHandleBase hObserved);
    void UpdateDamageReport(bool isClear);
};