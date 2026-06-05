#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ICSViewPlayerModule;
class PlayerHub final :public CSModuleBase {
    std::atomic<bool> ShowCompanionWindow = false;
public:
    std::vector<ICSViewPlayerModule*> PlayerViewModules;

    std::atomic<Steam64UID> currentSteamId;
    std::atomic<CS2::ui8TeamNum> currentTeam;

    bool Init()override;
    bool Window(MulNX::UINode* node);
};