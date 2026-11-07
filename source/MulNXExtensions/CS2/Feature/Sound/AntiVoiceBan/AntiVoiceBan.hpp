#pragma once
#include <Intro/Intros.hpp>

class AntiVoiceBan final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkProcessVoiceBan{};
    bool Init()override;
};