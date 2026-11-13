#pragma once
#include <Intro/Intros.hpp>

class AntiVoiceBan final :public CSModule<AntiVoiceBan> {
    std::unique_ptr<MulNX::Hook> hkProcessVoiceBan{};
    bool Init()override;
};