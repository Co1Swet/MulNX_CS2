#pragma once
#include <Intro/Intros.hpp>
#include <unordered_set>

class DeathMsgController final :public CSModule<DeathMsgController> {
    using HudDeathNotice_HandlePlayerDeath_t = char*(*)(uintptr_t hudThis, uintptr_t event);
    std::unique_ptr<MulNX::Hook> hkHudDeathNotice_HandlePlayerDeath = nullptr;
    HudDeathNotice_HandlePlayerDeath_t pRawHudDeathNotice_HandlePlayerDeath = nullptr;

    std::atomic<float> fLifeTimeOverride = -1.0f;
    std::atomic<bool> bOverrideLifeTime = false;
    std::atomic<float> fLifeTimeModOverride = -1.0f;
    std::atomic<bool> bOverrideLifeTimeMod = false;

    // ---------- 击杀信息过滤（白名单） ----------
    std::atomic<bool> bFilterEnabled = false;
    std::atomic<bool> bMatchAttacker = true;
    std::atomic<bool> bMatchVictim = false;
    std::atomic<bool> bMatchAssister = false;
    std::unordered_set<Steam64UID> whitelist;

    void Menu();
    void HubPlayer(MulNX::Message* umsg);
    bool Init();
    void ProcessMsg(MulNX::Message& msg)override;
    bool ShouldShow(CS2::CGameEvent* pEvent);
    MulNX::Hook::Then OnPlayerDeath(RegContext* ctx);
};
