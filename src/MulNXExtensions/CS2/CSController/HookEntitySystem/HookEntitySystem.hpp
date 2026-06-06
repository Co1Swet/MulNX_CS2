#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class HookEntitySystem final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkAddEntity = nullptr;
    std::unique_ptr<MulNX::Hook> hkRemoveEntity = nullptr;
public:
    bool Init()override;
};