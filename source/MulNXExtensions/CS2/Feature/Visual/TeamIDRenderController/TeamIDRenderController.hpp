#pragma once
#include <Intro/CSModuleBase.hpp>

class TeamIDRenderController final : public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPosTeamID_CmpForHide = nullptr;
    std::atomic<bool> hideEnemyTeamID = true;
    MulNX::Hook::Then HandleForShowTeamID(CS2::C_CSPlayerPawn* pCSPlayerPawn);

    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};