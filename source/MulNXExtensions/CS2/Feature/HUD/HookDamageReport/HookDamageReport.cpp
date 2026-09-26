#include "HookDamageReport.hpp"

namespace CS2 {
    enum class EKillTypes_t : uint8_t {
        KILL_NONE = 0x0,
        KILL_DEFAULT = 0x1,
        KILL_HEADSHOT = 0x2,
        KILL_BLAST = 0x3,
        KILL_BURN = 0x4,
        KILL_SLASH = 0x5,
        KILL_SHOCK = 0x6,
        KILLTYPE_COUNT = 0x7
    };

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

class CFakeDamageRecordPool {
    static constexpr size_t kMaxRecords = 64;
    CS2::CDamageRecord m_Records[kMaxRecords];
    int32_t        m_nCount = 0;

    CFakeDamageRecordPool() = default;
    CFakeDamageRecordPool(const CFakeDamageRecordPool&) = delete;
    CFakeDamageRecordPool& operator=(const CFakeDamageRecordPool&) = delete;
public:
    static CFakeDamageRecordPool& Get() {
        static CFakeDamageRecordPool s_Instance;
        return s_Instance;
    }

    void Clear() { m_nCount = 0; }
    int32_t Count() const { return m_nCount; }
    CS2::CDamageRecord* Data() { return m_Records; }

    void Add(const CS2::CHandleBase& hDamager,
        const CS2::CHandleBase& hRecipient,
        float flDamage,
        int32_t nNumHits,
        CS2::EKillTypes_t eKillType) {
        if (m_nCount >= kMaxRecords)
            return;

        auto* pRecord = &m_Records[m_nCount++];
        std::memset(pRecord, 0, sizeof(CS2::CDamageRecord));

        pRecord->m_hPlayerControllerDamager.value = hDamager.value;
        pRecord->m_hPlayerControllerRecipient.value = hRecipient.value;
        pRecord->m_flDamage = flDamage;
        pRecord->m_flActualHealthRemoved = flDamage;
        pRecord->m_iNumHits = nNumHits;
        pRecord->m_bIsOtherEnemy = true;
        pRecord->m_killType = eKillType;
    }
};

bool HookDamageReport::Init() {

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {

        auto tPos_Check_m_nSendUpdate = this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Hud::DamageReport::Pos_Check_m_nSendUpdate).Data();
        this->hkPos_Check_m_nSendUpdate = MulNX::Hook::Create(tPos_Check_m_nSendUpdate, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pDamageServices = reinterpret_cast<CS2::CCSPlayerController_DamageServices*>(ctx->rcx);
            auto* pNewValue = reinterpret_cast<int32_t*>(ctx->r8);

            if (pDamageServices->m_nSendUpdate == *pNewValue) {
                static int32_t s_FakeSendUpdate = 0;
                s_FakeSendUpdate = *pNewValue + 1;
                ctx->r8 = reinterpret_cast<uint64_t>(&s_FakeSendUpdate);
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_Check_m_nSendUpdate, "Pos_Check_m_nSendUpdate");

        auto tPos_GettedController = this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Hud::DamageReport::Pos_GettedController).Data();

        this->hkPos_GettedController = MulNX::Hook::Create(tPos_GettedController, [this](MulNX::Hook* hk, RegContext* ctx) {
            CS2::C_CSPlayerPawn* pObservingPawn = this->CS2Entitys->TryGetObservingPawn();
            if (!pObservingPawn)
                return MulNX::Hook::Then::Continue;

            auto* pObservingController = this->CS2Entitys
                ->GetBaseEntityFromHandle(MulNX::MRead(pObservingPawn->m_hController()))
                ->As<CS2::CCSPlayerController>();

            if (pObservingController) {
                ctx->rax = reinterpret_cast<uint64_t>(pObservingController);
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_GettedController, "Pos_GettedController");

        auto tFunc_UpdateDamageReport = this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Hud::DamageReport::Func_UpdateDamageReport).Data();
        this->hkFunc_UpdateDamageReport = MulNX::Hook::Create(tFunc_UpdateDamageReport, [this](MulNX::Hook* hk, RegContext* ctx) {

            auto* pObservingPawn = this->CS2Entitys->TryGetObservingPawn();
            if (!pObservingPawn)return MulNX::Hook::Then::Continue;

            auto* pObservedController = this->CS2Entitys
                ->GetBaseEntityFromHandle(MulNX::MRead(pObservingPawn->m_hController()))
                ->As<CS2::CCSPlayerController>();
            if (!pObservedController)return MulNX::Hook::Then::Continue;

            auto* pDamageServices = MulNX::MRead(pObservedController->m_pDamageServices());
            if (!pDamageServices)return MulNX::Hook::Then::Continue;

            auto hObserved = this->CS2Entitys->TryGetControllerHandle(pObservedController).value_or(CS2::CHandleBase());
            if (!hObserved.Valid())return MulNX::Hook::Then::Continue;
            
            // 替换
            auto origSize = pDamageServices->m_DamageList.m_nSize;
            auto origData = pDamageServices->m_DamageList.m_pData;
            this->RefreshPool(pObservedController, hObserved);
            auto& pool = CFakeDamageRecordPool::Get();
            pDamageServices->m_DamageList.m_nSize = pool.Count();
            pDamageServices->m_DamageList.m_pData = pool.Data();

            using RawFunc = __int64(__fastcall*)(__int64);
            auto r = hk->CallMaybeAs<RawFunc>(ctx->rcx);
            ctx->rax = static_cast<uint64_t>(r);

            // 恢复
            pDamageServices->m_DamageList.m_nSize = origSize;
            pDamageServices->m_DamageList.m_pData = origData;

            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkFunc_UpdateDamageReport, "Func_UpdateDamageReport");
        });

    return true;
}

void HookDamageReport::RefreshPool(CS2::CCSPlayerController* pObservedController, CS2::CHandleBase hObserved) {
    auto& pool = CFakeDamageRecordPool::Get();
    pool.Clear();

    auto observedTeam = MulNX::MRead(pObservedController->iTeamNum());
    int nSpecialCount = 0;

    for (int i = 0; i < this->CS2->client.dwGameEntitySystem_highestEntityIndex(); ++i) {
        auto* pController = this->CS2Entitys->GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!pController || !pController->IsPlayerController())continue;
        if (pController == pObservedController)continue;
        auto team = MulNX::MRead(pController->iTeamNum());
        if (team == observedTeam) continue;
        if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT)continue;

        auto hEnemy = this->CS2Entitys->TryGetControllerHandle(pController).value_or(CS2::CHandleBase());
        if (!hEnemy.Valid())continue;

        if (nSpecialCount == 0) {
            pool.Add(hEnemy, hObserved, 55.0f, 3, CS2::EKillTypes_t::KILL_DEFAULT);
            ++nSpecialCount;
        }
        else {
            pool.Add(hObserved, hEnemy, 88.0f, 5, CS2::EKillTypes_t::KILL_NONE);
        }
    }
}