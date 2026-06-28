#pragma once
#include <Intro/CSModuleBase.hpp>

class FlashRenderController final :public CSModuleBase {
    float* r_spectator_flashbang_opacity = nullptr;
    WrapHook hkDrawUp{};
    WrapHook hkDrawDown{};

    void Menu(MulNX::UINode* node);
    bool Init()override;
};