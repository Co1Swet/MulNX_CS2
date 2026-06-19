#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class HookManager final : public CSModuleBase {
private:
    MulNX::UISystem* pUISystem = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    HWND CS2hWnd = nullptr;

    std::unique_ptr<MulNX::Hook> hkD3D11CreateDevice = nullptr;
    std::unique_ptr<MulNX::Hook> hkCreateSwapChain = nullptr;
    
    // ClearDepthStencilView 钩子（清空前偷深度）
    std::unique_ptr<MulNX::Hook> hkClearDepthStencilView = nullptr;
    // Present 钩子
    std::unique_ptr<MulNX::Hook> hkPresent = nullptr;
    MulNX::Hook::Then D3D11AndImGuiInit(MulNX::Hook* hk, RegContext* ctx);
    MulNX::Hook::Then HandleOnPresent(MulNX::Hook* hk, RegContext* ctx);
    // ResizeBuffers 钩子
    std::unique_ptr<MulNX::Hook> hkResizeBuffers = nullptr;
    // 窗口过程钩子
    std::unique_ptr<MulNX::Hook> hkWndProc = nullptr;
    MulNX::Hook::Then HandleWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // 文件拖拽钩子
    std::unique_ptr<MulNX::Hook> hkDrop = nullptr;
    void HandleProcessDropFiles(IDataObject* pDataObj);

    void HookD3D11DeviceAndContext();
    void HookD3D11SwapChain(IDXGISwapChain* pSwapChain);
public:
    bool Init()override;
    void Deinit()override;
};