#pragma once
#include <Intro/HookGameEvents/HookGameEvents.hpp>

class DeathMsgController final :public CSModuleBase {
    using HashFunc_t = uint32_t * (*)(uint32_t* pResult, const char* pStr);
    using HandlePlayerDeath_t = void(*)(void* hudThis, void* event);
    HashFunc_t CSHashString{ nullptr };
    std::unique_ptr<MulNX::Hook> hkHandlePlayerDeath{ nullptr };

    uint32_t attacker_hash;
    uint32_t userid_hash;
    uint32_t assister_hash;

    bool Window(MulNX::UINode* node);
    MulNX::Hook::Then HandleOnPlayerDeath(CS2::CGameEvent* event);

    std::atomic<bool>enable{ false };
public:
    bool Init();
    void ProcessMsg(MulNX::Message& msg)override;
};