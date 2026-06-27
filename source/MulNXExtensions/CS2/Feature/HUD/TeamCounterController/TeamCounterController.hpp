#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class TeamCounterController final :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkTeamCounterWriteHP = nullptr;
    void Menu(MulNX::UINode* node);
public:
    bool Init()override;
};