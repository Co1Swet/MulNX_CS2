#pragma once
#include <Intro/CSModuleBase.hpp>

class ClientPanorama final :public CSModuleBase {
    using FindHudElement_t = uintptr_t(*)(const char* name);
    FindHudElement_t pFindHudElement = nullptr;
    bool Init()override;
public:
    uintptr_t FindHudElement(const char* name) {
        return this->pFindHudElement(name);
    }
};