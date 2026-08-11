#pragma once
#include <Intro/CSModuleBase.hpp>

class ReShowSpeaker final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_ifShowSpeaker = nullptr;
    bool Init()override;
};