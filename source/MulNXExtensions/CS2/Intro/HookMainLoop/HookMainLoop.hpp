#pragma once
#include <Intro/CSModuleBase.hpp>

class HookMainLoop final :public CSModule<HookMainLoop> {
    std::unique_ptr<MulNX::Hook> hkPos_Call_CInputService_ProcessCommands = nullptr;
    std::unique_ptr<MulNX::Hook> hkPos_PhysicsCreated = nullptr;
    std::unique_ptr<MulNX::Hook> hkQuit = nullptr;
    std::atomic<int> lastUpdateTick = 0;
    std::atomic<std::optional<int>> cachedTickJump;
    void OnMainLoop();

    bool Init()override;
};