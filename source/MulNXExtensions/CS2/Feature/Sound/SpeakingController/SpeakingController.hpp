#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/BackgroundEntityScan/EntityIterationMixin.hpp>

class SpeakingController :public CSModuleBase {
    int* tv_listen_voice_indices = nullptr;
    int bufferMask = 0;

    std::unordered_map<Steam64UID, bool>checkList{};

    std::atomic<bool> listMode = false;
    std::atomic<bool> autoActiveTeamVoice = false;
    std::atomic<bool> onlyCurOBingSameTeam = false;
    void OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn);
    bool CheckState(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn)const;

    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void Main();
};