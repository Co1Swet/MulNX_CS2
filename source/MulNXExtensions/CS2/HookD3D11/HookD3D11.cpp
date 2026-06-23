#include "HookD3D11.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>

bool HookD3D11::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::UISystem>("UISystem");
    this->pGraphicsManager = this->Core->ModuleManager()->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    this->SubscribeSync("Hook/LoadLibraryExW/d3d11.dll", [this](MulNX::Message& msg) {
        auto pD3D11CreateDevice = (uint8_t*)GetProcAddress(GetModuleHandleW(L"d3d11.dll"), "D3D11CreateDevice");
        this->hkD3D11CreateDevice = MulNX::Hook::Create(pD3D11CreateDevice, [this](MulNX::Hook* hk, RegContext* ctx) {
            this->hkD3D11CreateDevice->Detach();
            // 暂存参数
            auto ppDevice = *hk->GetStackParam<ID3D11Device**>(ctx, 7);
            auto ppDeviceContext = *hk->GetStackParam<ID3D11DeviceContext**>(ctx, 9);
            // 保存D3D11环境信息，供后续使用
            this->pGraphicsManager->D3D11Cfg.pAdapter = (IDXGIAdapter*)ctx->rcx;
            this->pGraphicsManager->D3D11Cfg.DriverType = *(D3D_DRIVER_TYPE*)&ctx->rdx;
            this->pGraphicsManager->D3D11Cfg.Software = *(HMODULE*)&ctx->r8;
            this->pGraphicsManager->D3D11Cfg.Flags = *(UINT*)&ctx->r9;
            this->pGraphicsManager->D3D11Cfg.SDKVersion = *hk->GetStackParam<UINT>(ctx, 6);

            hk->CallMaybeOrigin(6, ctx);
            this->pGraphicsManager->pd3dDevice = *ppDevice;
            this->pGraphicsManager->pd3dContext = *ppDeviceContext;
            this->HookD3D11DeviceAndContext();
            return MulNX::Hook::Then::Return;
            }).value();
        this->hkD3D11CreateDevice->Attach();
        this->LogSucc(I18n("hook.attached", "D3D11CreateDevice"));
        });
    return true;
}
void HookD3D11::HookD3D11DeviceAndContext() {
    IDXGIFactory* pFactory = nullptr;
    this->pGraphicsManager->D3D11Cfg.pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pFactory);
    this->hkCreateSwapChain = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pFactory)->GetVFuncPtr(10), [this](MulNX::Hook* hk, RegContext* ctx) {
        this->hkCreateSwapChain->Detach();
        IDXGISwapChain** ppSwapChain = (IDXGISwapChain**)ctx->r9;
        hk->CallMaybeOrigin(0, ctx);
        this->HookD3D11SwapChain(*ppSwapChain);
        return MulNX::Hook::Then::Return;
        }).value();
    this->hkCreateSwapChain->Attach();
    this->LogSucc(I18n("hook.attached", "CreateSwapChain"));
    // ---- Hook ClearDepthStencilView (vtable index 53) ----
    this->hkClearDepthStencilView = MulNX::Hook::Create((uint8_t*)IVClass::Assume(this->pGraphicsManager->pd3dContext)->GetVFuncPtr(53), [this](MulNX::Hook* hk, RegContext* ctx) {
        ID3D11DeviceContext* pCtx = (ID3D11DeviceContext*)ctx->rcx;
        ID3D11DepthStencilView* pDSV = (ID3D11DepthStencilView*)ctx->rdx;
        UINT ClearFlags = *(UINT*)(&ctx->r8);
        this->pGraphicsManager->OnClearDepthStencilView(pCtx, pDSV, ClearFlags);
        return MulNX::Hook::Then::Continue;
        }).value();
    this->hkClearDepthStencilView->Attach();
    this->LogSucc(I18n("hook.attached", "ClearDepthStencilView"));
}
void HookD3D11::HookD3D11SwapChain(IDXGISwapChain* pSwapChain) {
    // Hook Present函数
    // 函数开头：
    // 0~4：Steam钩子（OBS游戏捕获钩子会与其交互，进行画面捕获）
    // 5~9：在这里部署MulNX的钩子，注意此时OBS捕获已经完成，可以做到启动顺序无关的渲染分离
    // 10+：其它汇编指令，我们的MulNX钩子最终跳转继续执行
    this->hkPresent = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pSwapChain)->GetVFuncPtr(8) + 5, [this](MulNX::Hook* hk, RegContext* ctx) {
        this->PublishSync("Hook/Present/First"_hash);
        hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->D3D11AndImGuiInit(hk, ctx);});
        return MulNX::Hook::Then::Continue;
        }).value();
    this->hkPresent->Attach();
    this->LogSucc(I18n("hook.attached", "Present"));
    // ---- Hook ResizeBuffers (vtable index 13) ----
    this->hkResizeBuffers = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pSwapChain)->GetVFuncPtr(13), [this](MulNX::Hook* hk, RegContext* ctx) {
        this->pGraphicsManager->ReleaseOld();
        return MulNX::Hook::Then::Continue;
        }).value();
    this->hkResizeBuffers->Attach();
    this->LogSucc(I18n("hook.attached", "ResizeBuffers"));

    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    MulNX::Message msg("Hook/hWnd"_hash);
    msg.p1.as<HWND>() = sd.OutputWindow;
    this->PublishSync(msg);
}
MulNX::Hook::Then HookD3D11::D3D11AndImGuiInit(MulNX::Hook* hk, RegContext* ctx) {
    hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->HandleOnPresent(hk, ctx);});
    // ImGui 初始化
    ImGui_ImplDX11_Init(this->pGraphicsManager->pd3dDevice, this->pGraphicsManager->pd3dContext);
    // 创建绿幕着色器资源
    this->pGraphicsManager->ReleaseOld();
    this->pGraphicsManager->CreateGreenScreenAssets();

    this->pUISystem->FrameBefore = [this]() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        return true;
        };
    this->pUISystem->FrameBehind = [this]() {
        ImGui::EndFrame();
        ImGui::Render();
        this->pGraphicsManager->pd3dContext->OMSetRenderTargets(1, &this->pGraphicsManager->view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        };
    this->Core->Driver()->CreateMainDraw();
    this->pGlobalVars->SystemReady.store(true);
    return MulNX::Hook::Then::Continue;
}
MulNX::Hook::Then HookD3D11::HandleOnPresent(MulNX::Hook* hk, RegContext* ctx) {
    this->pGraphicsManager->pSwapChain = (IDXGISwapChain*)ctx->rcx;
    // 绿幕渲染
    this->pGraphicsManager->BuildNew();
    this->pGraphicsManager->OnPresent();
    // UI 系统渲染
    this->pUISystem->HandleUpdate();
    this->pUISystem->Render();
    return MulNX::Hook::Then::Continue;
}
void HookD3D11::Deinit() {
    this->hkClearDepthStencilView->Detach();
    this->hkPresent->Detach();
}