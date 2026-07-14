#pragma once
#include <Intro/CSModuleBase.hpp>

class AntiVoiceBan final :public CSModuleBase {
    WrapHook hkProcessVoiceBan{};
    bool Init()override;
};