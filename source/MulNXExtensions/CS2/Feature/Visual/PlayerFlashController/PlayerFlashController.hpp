#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/BackgroundEntityScan/EntityIterationMixin.hpp>

class PlayerFlashController final :public CSModuleBase, public EntityIterationMixin<PlayerFlashController> {
    std::atomic<bool>bForceNoFlash = false;
    void OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn)override;
    bool Init()override;
    bool Menu();
};