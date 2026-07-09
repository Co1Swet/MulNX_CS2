#pragma once
#include <Intro/CSModuleBase.hpp>

class SoundCircleFix final :public CSModuleBase {
    WrapHook hkPos_CallGetPawnUpdateCirclePos{};
    WrapHook hkPos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque{};
    WrapHook hkPos_CallGetPawnMaybeOtherAsyncSoundEnque{};
    bool Init()override;
};