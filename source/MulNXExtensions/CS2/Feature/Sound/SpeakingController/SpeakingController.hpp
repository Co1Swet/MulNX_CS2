#pragma once
#include <Intro/CSModuleBase.hpp>

class SpeakingController :public CSModuleBase {
    int* tv_listen_voice_indices = nullptr;
    int bufferMask = 0;
    CS2::ui8TeamNum targetTeam{};
    std::atomic<bool>onlyCurOBingSameTeam = false;
    void OnItBegin()override;
    void OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn)override;
    void OnItEnd()override;

    void Menu();
    bool Init()override;
};