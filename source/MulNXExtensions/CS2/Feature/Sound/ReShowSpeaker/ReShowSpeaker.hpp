#pragma once
#include <Intro/CSModuleBase.hpp>

class ReShowSpeaker final :public CSModuleBase {
    using UpdateSpeakerStatus_t = int64_t(*)(int64_t voiceStatus, uint32_t slot, int, uint8_t speaking);
    using GetVoiceStatus_t = int64_t(*)();

    std::atomic<int> lastUpdateTick = 0;
    UpdateSpeakerStatus_t pFuncUpdateSpeakerStatus = nullptr;
    GetVoiceStatus_t pFuncGetVoiceStatus = nullptr;

    std::unique_ptr<MulNX::Hook> hkPos_ifShowSpeaker = nullptr;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};