#pragma once
#include <Intro/CSModuleBase.hpp>

class SkinController final :public CSModuleBase {
    using RegenerateWeaponSkins = void(*)(void*);
    RegenerateWeaponSkins regenerateWeaponSkins = nullptr;
    void Window(MulNX::UINode* node);
    void Apply();
    std::atomic<int> targetIndex = 1;
    std::atomic<bool> legacyModel = false;
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};