#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <MulNXUtils/MemInsights/RetEditor/RetEditor.hpp>

class HookD3D11 final : public MulNX::Module<HookD3D11>, public HookMixin<HookD3D11> {
    MulNX::Memory::DllModule rendersystemdx11{};
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    std::atomic<bool> needUpdate = true;

    std::unique_ptr<MulNX::Hook> hkPosCallPresent{};
    // ClearDepthStencilView 钩子（清空前偷深度）
    std::unique_ptr<MulNX::Hook> hkClearDepthStencilView{};
    // Present 钩子
    std::unique_ptr<MulNX::Hook> hkPresent{};
    MulNX::Hook::Then D3D11Init(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then HandleOnPresent(MulNX::Hook* hk, RegContext* ctx);
    // ResizeBuffers 钩子
    std::unique_ptr<MulNX::Hook> hkResizeBuffers{};
    MulNX::Hook::Then HandleOnResizeBuffers(MulNX::Hook* hk, RegContext* ctx);

    void UpdateRenderXY(IDXGISwapChain* pSwapChain);
    void HookD3D11DeviceAndContext();
    void HookD3D11SwapChain(IDXGISwapChain* pSwapChain);

    bool Init()override;
};