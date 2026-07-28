#pragma once
#include <Intro/CSModuleBase.hpp>

class FlashRenderController final :public CSModuleBase {
    float* r_spectator_flashbang_opacity = nullptr;
    std::unique_ptr<MulNX::Hook> hkDrawUp{};
    std::unique_ptr<MulNX::Hook> hkDrawDown{};
    std::atomic<bool>rendFlashUpHUD = true;
    std::atomic<bool>rendFlashDownHUD = false;

    void Menu();
    bool Init()override;
};