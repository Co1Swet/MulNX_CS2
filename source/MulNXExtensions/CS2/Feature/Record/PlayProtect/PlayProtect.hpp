#pragma once
#include <Intro/CSModuleBase.hpp>

class PlayProtect final : public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkCCSGO_EndOfMatch_Activate = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_CDemoPlayer_InternalReadPacket_CheckFlag_DemoPauseAtEnd = nullptr;

    MulReflBool(preventEndOfMatch, true);
    MulReflBool(forceDemoPauseAtEnd, true);

    void Menu();
    bool Init()override;
};