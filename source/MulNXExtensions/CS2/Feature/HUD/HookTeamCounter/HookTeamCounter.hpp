#pragma once
#include <Intro/Intros.hpp>

class HookTeamCounter final :public CSModule<HookTeamCounter> {
    std::unique_ptr<MulNX::Hook> hkTeamCounterFillPlayerSlotCache = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_UpdatePanoramaFullInfoVisible = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_UpdatePanoramaNameVisible = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_UpdatePanoramaNameVisible2 = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_UpdatePanoramaSpecTargetVisible = nullptr;

    MulReflBool(hideEnemyHP, true);
    MulReflBool(hideEnemyDefuseOrKit, true);
    MulReflBool(showEquipmentInfo, true);
    MulReflBool(hideEnemyEquipmentInfo, true);
    MulReflBool(hideEquipmentInfoWhenActive, true);
    MulReflBool(showName, true);
    MulReflBool(hideNameWhenActive, true);

    void Menu();
    bool Init()override;
    void OnClient();
    void HandleTeamCounterFillPlayerSlotCacheHook(MulNX::Hook* hk, RegContext* ctx);
};