#pragma once
#include <Intro/CSModuleBase.hpp>

class CS2Hash final :public CSModuleBase {
    using HashFunc_t = uint32_t(__fastcall*)(const char* str, size_t len, uint32_t seed);
    HashFunc_t CSHashString{ nullptr };

    bool Init()override;
public:
    uint32_t attacker = 0;
    uint32_t userid = 0;
    uint32_t assister = 0;
    uint32_t hitgroup = 0;
};