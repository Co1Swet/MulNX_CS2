#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>

class TeamCounterController final :public CSViewPlayerModuleBase {
    std::unique_ptr<MulNX::Hook>hkTeamCounterWriteHP = nullptr;
    void HubWindow(MulNX::UINode* node)override;
public:
    bool Init()override;
};