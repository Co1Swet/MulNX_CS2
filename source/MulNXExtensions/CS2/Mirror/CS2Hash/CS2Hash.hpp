#pragma once
#include <Intro/CSModuleBase.hpp>

class CS2Hash final :public CSModuleBase {
    using HashFunc_t = uint32_t(__fastcall*)(const char* str, uint32_t len, uint32_t seed);
    HashFunc_t CSHashString{ nullptr };
    uint32_t WrapHash(const std::string& str);
    bool Init()override;
public:
    uint32_t attacker = 0;
    uint32_t userid = 0;
    uint32_t assister = 0;
    uint32_t hitgroup = 0;

    uint32_t dmg_health = 0;
    uint32_t health = 0;

    uint32_t assistedflash = 0;
    uint32_t weapon = 0;
    uint32_t headshot = 0;
    uint32_t penetrated = 0;
    uint32_t noscope = 0;
    uint32_t thrusmoke = 0;
    uint32_t attackerblind = 0;
    uint32_t attackerinair = 0;
};