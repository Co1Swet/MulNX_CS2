#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>

class ImGuiBinder final :public CSModuleBase {
    MulNX::UISystem* pUISystem = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    bool Init()override;
    void ImGuiInit();
};