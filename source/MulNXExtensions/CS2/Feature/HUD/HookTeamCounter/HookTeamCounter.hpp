#pragma once
#include <Intro/Intros.hpp>

class HookTeamCounter final :public CSModule<HookTeamCounter> {
    std::unique_ptr<MulNX::Hook>hkTeamCounterFillPlayerSlotCache = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_UpdatePanoramaFullInfoVisible = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_UpdatePanoramaNameVisible = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_UpdatePanoramaNameVisible2 = nullptr;
    std::unique_ptr<MulNX::Hook>hkPos_UpdatePanoramaSpecTargetVisible = nullptr;
    MulReflBool(hideEnemyHP, true);
    MulReflBool(hideEnemyDefuseOrKit, true);
    MulReflBool(forceHideEquipmentInfo, false);
    MulReflBool(noHideShowName, false);

    void Menu();
    bool Init()override;
    void HandleTeamCounterFillPlayerSlotCacheHook(MulNX::Hook* hk, RegContext* ctx);
};