#pragma once
#include <Intro/CSModuleBase.hpp>

class CS2Hash final :public CSModuleBase {
    using HashFunc_t = uint32_t * (*)(uint32_t* pResult, const char* pStr);
    HashFunc_t CSHashString{ nullptr };

    bool Init()override;
public:
    uint32_t attacker = 0;
    uint32_t userid = 0;
    uint32_t assister = 0;
    uint32_t hitgroup = 0;
};