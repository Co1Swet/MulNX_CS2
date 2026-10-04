#pragma once
#include <Intro/CSModuleBase.hpp>

class HookMainLoop final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkPos_Call_CInputService_ProcessCommands = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_PhysicsCreated = nullptr;
    std::atomic<int> lastUpdateTick = 0;
    
    bool Init()override;
};