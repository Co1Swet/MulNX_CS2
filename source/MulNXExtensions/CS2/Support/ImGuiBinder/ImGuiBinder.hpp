#pragma once
#include <Intro/Intros.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>

class ImGuiBinder final :public CSModule<ImGuiBinder> {
    MulNX::UISystem* pUISystem = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    bool Init()override;
    void ImGuiInit();
};