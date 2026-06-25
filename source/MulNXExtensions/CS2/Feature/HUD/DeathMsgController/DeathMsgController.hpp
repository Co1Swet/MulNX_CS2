#pragma once
#include <Intro/CSModuleBase.hpp>

struct KillEvent {
    int DemoTick;
    // float time;
    uint64_t attackerSteamId = 0;
    uint64_t victimSteamId = 0;
    uint64_t assisterSteamId = 0;
    // std::string weapon;
    // bool headshot;
    // bool penetrated;
    // bool noscope;
    // bool thrusmoke;
    // bool attackerblind;
    // 可选位置
    // float attackerPos[3];
    // float victimPos[3];
};

class DeathMsgController final :public CSModuleBase {
    using HashFunc_t = uint32_t * (*)(uint32_t* pResult, const char* pStr);
    using HandlePlayerDeath_t = void(*)(void* hudThis, void* event);
    HashFunc_t CSHashString{ nullptr };
    std::unique_ptr<MulNX::Hook> hkHandlePlayerDeath{ nullptr };

    uint32_t attacker_hash;
    uint32_t userid_hash;
    uint32_t assister_hash;

    bool Window(MulNX::UINode* node);
    MulNX::Hook::Then HandleOnPlayerDeath(void* event);

    std::atomic<bool>enable{ false };
public:
    bool Init();
    void ProcessMsg(MulNX::Message& msg)override;
};