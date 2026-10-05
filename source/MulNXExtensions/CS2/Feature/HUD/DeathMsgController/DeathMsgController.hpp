#pragma once
#include <Intro/HookGameEvents/HookGameEvents.hpp>

class DeathMsgController final :public CSModuleBase {
    using HudDeathNotice_HandlePlayerDeath_t = char*(*)(uintptr_t hudThis, uintptr_t event);
    std::unique_ptr<MulNX::Hook> hkHudDeathNotice_HandlePlayerDeath = nullptr;
    HudDeathNotice_HandlePlayerDeath_t pRawHudDeathNotice_HandlePlayerDeath = nullptr;

    std::atomic<float> fLifeTimeOverride = -1.0f;
    std::atomic<bool> bOverrideLifeTime = false;
    std::atomic<float> fLifeTimeModOverride = -1.0f;
    std::atomic<bool> bOverrideLifeTimeMod = false;

    void Menu();
    bool Init();
    void ProcessMsg(MulNX::Message& msg)override;
    MulNX::Hook::Then OnPlayerDeath(RegContext* ctx);
};