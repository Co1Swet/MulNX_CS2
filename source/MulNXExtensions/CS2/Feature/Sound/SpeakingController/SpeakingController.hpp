#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/BackgroundEntityScan/EntityIterationMixin.hpp>
#include <MulNX/Systems/VarReflSystem/VarReflSystem.hpp>

class SpeakingController :public CSModuleBase {
    int* tv_listen_voice_indices = nullptr;
    int* tv_listen_voice_indices_h = nullptr;

    int bufferMask = 0;
    int bufferMaskHigh = 0;

    std::unordered_map<Steam64UID, bool>checkList{};

    MulReflBool(listMode, false);
    // std::atomic<bool> listMode = false;
    // MulNX::RegisterVar re{ this,this->listMode ,"listMode" };
    std::atomic<bool> autoActiveTeamVoice = false;
    std::atomic<bool> onlyCurOBingSameTeam = false;
    void OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn);
    bool CheckState(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn)const;

    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void Main();
};