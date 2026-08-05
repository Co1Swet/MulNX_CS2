#pragma once
#include <Intro/CSModuleBase.hpp>

class TeamCounterController final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkTeamCounterFillPlayerSlotCache = nullptr;
    std::atomic<bool> hideEnemyHP = true;
    std::atomic<bool> hideEnemyDefuseOrKit = true;

    void Menu();
    bool Init()override;
    void HandleTeamCounterFillPlayerSlotCacheHook(MulNX::Hook* hk, RegContext* ctx);
};