#pragma once
#include <Intro/CSModuleBase.hpp>

class PlayerHub final :public CSModuleBase {
    std::atomic<bool> ShowCompanionWindow = false;
public:
    std::vector<class ICSViewPlayerModule*> PlayerViewModules;

    std::atomic<Steam64UID> currentSteamId;
    std::atomic<CS2::ui8TeamNum> currentTeam;

    bool Init()override;
    bool Window(MulNX::UICoordinator* uico);
};