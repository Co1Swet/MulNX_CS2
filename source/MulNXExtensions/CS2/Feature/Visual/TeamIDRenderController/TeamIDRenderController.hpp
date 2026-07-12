#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class TeamIDRenderController final : public CSViewPlayerModuleBase {
    void Menu();
    std::unique_ptr<MulNX::Hook> hkPosTeamID_CmpForHide = nullptr;

    MulNX::Hook::Then HandleForShowTeamID(CS2::C_CSPlayerPawn* pCSPlayerPawn);

    bool Init() override;
};