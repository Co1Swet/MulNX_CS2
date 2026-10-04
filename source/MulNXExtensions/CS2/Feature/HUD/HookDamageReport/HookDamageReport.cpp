#include "HookDamageReport.hpp"
#include "CFakeDamageRecordPool.hpp"

void HookDamageReport::Menu() {
    MulNX::UI::Checkbox("启用伤害报告增强", this->enable);
}

bool HookDamageReport::Init() {
    this->pClientPanorama = this->FindModule<ClientPanorama>("ClientPanorama");
    this->pDamageRecorder = this->FindModule<DamageRecorder>("DamageRecorder");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto tPos_Check_m_nSendUpdate = this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Hud::DamageReport::Pos_Check_m_nSendUpdate).Data();
        this->hkPos_Check_m_nSendUpdate = MulNX::Hook::Create(tPos_Check_m_nSendUpdate, [this](MulNX::Hook* hk, RegContext* ctx) {
            this->needUpdate = true;
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
        this->pFunc_UpdateDamageReport = reinterpret_cast<UpdateDamageReport_t>(tFunc_UpdateDamageReport);
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

            auto r = hk->CallMaybeAs<UpdateDamageReport_t>(ctx->rcx);
            ctx->rax = r;
            // 恢复
            pDamageServices->m_DamageList.m_nSize = origSize;
            pDamageServices->m_DamageList.m_pData = origData;
            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkFunc_UpdateDamageReport, "Func_UpdateDamageReport");

        this->pFunc_DispatchClearAllPostRoundDamageReportPanels =
            std::bit_cast<DispatchClearAllPostRoundDamageReportPanels_t>(this->CS2->client.GetTextRegion()
                .FindRegion(CS2::Signatures::Hud::DamageReport::Pos_Call_DispatchClearAllPostRoundDamageReportPanels)
                .TryGetCallTarget());
        });

    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...) {
        if (!this->needUpdate)return;
        this->needUpdate = false;
        if (this->enable == false)return;
        this->UpdateDamageReport(false);
        });

    this->SubscribeSync("Hook/MainLoop/TickJumpDetected", [this](auto&&...) {
        if (this->enable == false)return;
        this->UpdateDamageReport(true);
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {
        this->Menu();
        });

    return true;
}

void HookDamageReport::UpdateDamageReport(bool isClear) {
    auto pCCSGO_HudTeamCounter = this->pClientPanorama->FindHudElement("CCSGO_HudTeamCounter");
    auto v5 = pCCSGO_HudTeamCounter - 32;
    if (!pCCSGO_HudTeamCounter)
        v5 = 0;
    if (isClear) {
        this->pFunc_DispatchClearAllPostRoundDamageReportPanels(v5);
    }
    else {
        auto r = this->pFunc_UpdateDamageReport(v5);
    }
}

namespace {
    struct EnemyAgg {
        float damage = 0.0f;
        int   hits = 0;
        CS2::EKillTypes_t killType = CS2::EKillTypes_t::KILL_NONE;
    };
}

void HookDamageReport::RefreshPool(CS2::CCSPlayerController* pObservedController, CS2::CHandleBase hObserved) {
    auto& pool = CFakeDamageRecordPool::Get();
    pool.Clear();

    auto observedTeam = MulNX::MRead(pObservedController->iTeamNum());
    auto observedSteamId = MulNX::MRead(pObservedController->m_steamID());
    auto round = this->CS2->client.dwGameRules()->m_nRoundStartCount;

    auto gaveHits = this->pDamageRecorder->GetPlayerGiveDamageInfo(round, observedSteamId);
    auto tookHits = this->pDamageRecorder->GetPlayerTakeDamageInfo(round, observedSteamId);

    std::map<Steam64UID, EnemyAgg> gaveAgg;
    for (const auto& h : gaveHits) {
        auto& agg = gaveAgg[h.victim];
        agg.damage += static_cast<float>(h.damage);
        agg.hits += 1;
        if (h.isKill) {
            agg.killType = CS2::TranslateKillType(h.damageType);
        }
    }

    std::map<Steam64UID, EnemyAgg> tookAgg;
    for (const auto& h : tookHits) {
        auto& agg = tookAgg[h.attacker];
        agg.damage += static_cast<float>(h.damage);
        agg.hits += 1;
        if (h.isKill) {
            agg.killType = CS2::TranslateKillType(h.damageType);
        }
    }

    for (const auto& [enemyId, agg] : gaveAgg) {
        auto* pEnemy = this->CS2Entitys->FindControllerBySteam64UID(enemyId);
        if (!pEnemy) continue;
        if (MulNX::MRead(pEnemy->iTeamNum()) == observedTeam) continue;
        auto hEnemy = this->CS2Entitys->TryGetControllerHandle(pEnemy).value_or(CS2::CHandleBase());
        if (!hEnemy.Valid()) continue;

        pool.Add(hObserved, hEnemy, agg.damage, agg.hits, agg.killType);
    }

    for (const auto& [enemyId, agg] : tookAgg) {
        auto* pEnemy = this->CS2Entitys->FindControllerBySteam64UID(enemyId);
        if (!pEnemy) continue;
        if (MulNX::MRead(pEnemy->iTeamNum()) == observedTeam) continue;
        auto hEnemy = this->CS2Entitys->TryGetControllerHandle(pEnemy).value_or(CS2::CHandleBase());
        if (!hEnemy.Valid()) continue;

        pool.Add(hEnemy, hObserved, agg.damage, agg.hits, agg.killType);
    }
}