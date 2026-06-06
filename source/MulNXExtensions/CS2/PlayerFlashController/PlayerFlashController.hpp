#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class CSController;
class PlayerFlashController final :public CSModuleBase {
private:
    std::atomic<bool>bForceNoFlash = false;
public:
    bool Init()override;
    bool Menu(MulNX::UINode* node);
    bool HandleForceFlash(CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn);
};