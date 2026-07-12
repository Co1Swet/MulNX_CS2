#pragma once
#include <Intro/CSModuleBase.hpp>

class TeamCounterController final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkTeamCounterWriteHP = nullptr;
    void Menu();
    bool Init()override;
};