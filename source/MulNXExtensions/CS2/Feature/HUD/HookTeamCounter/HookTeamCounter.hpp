#pragma once
#include <Intro/CSModuleBase.hpp>

class HookTeamCounter final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkTeamCounterFillPlayerSlotCache = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_UpdatePanoramaFullInfoVisible = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_UpdatePanoramaNameVisible = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_UpdatePanoramaSpecTargetVisible = nullptr;
    std::atomic<bool> hideEnemyHP = true;
    std::atomic<bool> hideEnemyDefuseOrKit = true;
    std::atomic<bool> forceHideEquipmentInfo = false;
    std::atomic<bool> forceShowName = false;

    void Menu();
    bool Init()override;
    void HandleTeamCounterFillPlayerSlotCacheHook(MulNX::Hook* hk, RegContext* ctx);
};