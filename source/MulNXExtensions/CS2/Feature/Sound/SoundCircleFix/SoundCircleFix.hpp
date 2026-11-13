#pragma once
#include <Intro/Intros.hpp>

class SoundCircleFix final :public CSModule<SoundCircleFix> {
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnUpdateCirclePos{};
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque{};
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnMaybeOtherAsyncSoundEnque{};
    MulReflBool(enable, true);
    bool Init()override;
};