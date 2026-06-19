#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class PlayerFlashController final :public CSModuleBaseT<PlayerFlashController> {
private:
    std::atomic<bool>bForceNoFlash = false;
    void OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn)override;
public:
    bool Init()override;
    bool Menu(MulNX::UINode* node);
    
};