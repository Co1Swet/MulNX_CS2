#pragma once
#include <Intro/CSModuleBase.hpp>

class SoundCircleFix final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnUpdateCirclePos{};
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque{};
    std::unique_ptr<MulNX::Hook> hkPos_CallGetPawnMaybeOtherAsyncSoundEnque{};
    bool Init()override;
};