#pragma once
#include <Intro/CSModuleBase.hpp>

class PlayerVolumeController final :public CSModuleBase {
    MulNX::Memory::DllModule soundsystem{};
    WrapHook hkSoundSystem001_SetPlayerVoiceVolume{};
    WrapHook hkSoundSystem001_GetPlayerVoiceVolume{};
    bool Init()override;
};