#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class PlayerFlashController final :public CSModuleBase {
    static constexpr bool ParticipateIt = true;
private:
    std::atomic<bool>bForceNoFlash = false;
public:
    bool Init()override;
    bool Menu(MulNX::UINode* node);
    bool HandleForceFlash(CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn);
};