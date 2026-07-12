#pragma once
#include <Intro/CSModuleBase.hpp>

class TeamIDRenderController final : public CSModuleBase {
    void Menu();
    std::unique_ptr<MulNX::Hook> hkPosTeamID_CmpForHide = nullptr;

    MulNX::Hook::Then HandleForShowTeamID(CS2::C_CSPlayerPawn* pCSPlayerPawn);

    bool Init() override;
};