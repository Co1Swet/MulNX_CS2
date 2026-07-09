#pragma once
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <Intro/CSModuleBase.hpp>
#include <MulNXUtils/MemInsights/RetEditor/RetEditor.hpp>

class HookD3D11 final : public MulNX::Module<HookD3D11>, public HookMixin<HookD3D11> {
    MulNX::Memory::DllModule gameoverlayrenderer64{};

    MulNX::UISystem* pUISystem = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    HWND hCS2Wnd = nullptr;

    WrapHook hkPosCallPresent{};
    
    // ClearDepthStencilView 钩子（清空前偷深度）
    WrapHook hkClearDepthStencilView{};
    // Present 钩子
    WrapHook hkPresent{};
    MulNX::Hook::Then D3D11AndImGuiInit(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then HandleOnPresent(MulNX::Hook* hk, RegContext* ctx);
    // ResizeBuffers 钩子
    WrapHook hkResizeBuffers{};

    void HookD3D11DeviceAndContext();
    void HookD3D11SwapChain(IDXGISwapChain* pSwapChain);

    bool Init()override;
    void Deinit()override;
};