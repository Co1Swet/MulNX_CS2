#pragma once
#include <Intro/CSModuleBase.hpp>

class HookEntitySystem final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkAddEntity = nullptr;
    std::unique_ptr<MulNX::Hook> hkRemoveEntity = nullptr;
public:
    bool Init()override;
};