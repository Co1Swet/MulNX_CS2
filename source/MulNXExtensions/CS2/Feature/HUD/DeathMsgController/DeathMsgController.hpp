#pragma once
#include <Intro/HookGameEvents/HookGameEvents.hpp>

class DeathMsgController final :public CSModuleBase {
    using HandlePlayerDeath_t = void(*)(void* hudThis, void* event);
    std::unique_ptr<MulNX::Hook> hkHandlePlayerDeath{ nullptr };

    bool Window(MulNX::UINode* node);
    MulNX::Hook::Then HandleOnPlayerDeath(CS2::CGameEvent* event);

    std::atomic<bool>enable{ false };
public:
    bool Init();
    void ProcessMsg(MulNX::Message& msg)override;
};