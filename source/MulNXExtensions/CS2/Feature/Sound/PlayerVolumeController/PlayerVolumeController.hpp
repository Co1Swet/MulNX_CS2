#pragma once
#include <Intro/CSModuleBase.hpp>

class PlayerVolumeController final :public CSModuleBase {
    MulNX::Memory::DllModule soundsystem{};
    std::unique_ptr<MulNX::Hook> hkSoundSystem001_SetPlayerVoiceVolume{};
    std::unique_ptr<MulNX::Hook> hkSoundSystem001_GetPlayerVoiceVolume{};
    bool Init()override;
};