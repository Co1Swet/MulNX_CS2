#include "HookD3D11.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <wrl/client.h>

static const GUID IID_UnwrappedObject =
{ 0x7f2c9a11, 0x3b4e, 0x4d6a, { 0x81, 0x2f, 0x5e, 0x9c, 0xd3, 0x7a, 0x1b, 0x42 } };

bool HookD3D11::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::UISystem>("UISystem");
    this->pGraphicsManager = this->Core->ModuleManager()->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    this->SubscribeSync("Hook/LoadLibraryExW/rendersystemdx11.dll", [this](MulNX::Message& msg) {
        this->rendersystemdx11 = MulNX::Memory::DllModule(L"rendersystemdx11.dll");
        auto target = this->rendersystemdx11.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Render::Pos_Call_Present).Data();

        this->hkPosCallPresent = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pSwapChain = (IDXGISwapChain*)ctx->rcx;
            static int i = 0;
            if (++i < 64)return MulNX::Hook::Then::Continue;
            this->hkPosCallPresent->Detach();
            IDXGISwapChain* pRealSwapChain = pSwapChain;
            // 如果加载了reshade，这里拿原始对象
            HRESULT hr = pSwapChain->QueryInterface(IID_UnwrappedObject, (void**)&pRealSwapChain);
            this->HookD3D11SwapChain(pRealSwapChain);
            if (SUCCEEDED(hr)) {
                this->LogWarning("检测到ReShade加载！已经反查原始交换链对象并部署钩子！");
                pRealSwapChain->Release();
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPosCallPresent, "PosCallPresent");
        });

    return true;
}
void HookD3D11::UpdateRenderXY(IDXGISwapChain* pSwapChain) {
    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    this->pGlobalVars->renderX.store(sd.BufferDesc.Width, std::memory_order_release);
    this->pGlobalVars->renderY.store(sd.BufferDesc.Height, std::memory_order_release);
    this->LogInfo(std::format("当前渲染分辨率：{} x {}", sd.BufferDesc.Width, sd.BufferDesc.Height));
}
void HookD3D11::HookD3D11DeviceAndContext() {
    this->hkClearDepthStencilView = MulNX::Hook::Create((uint8_t*)IVClass::Assume(this->pGraphicsManager->pd3dContext)->GetVFuncPtr(53), [this](MulNX::Hook* hk, RegContext* ctx) {
        ID3D11DeviceContext* pCtx = (ID3D11DeviceContext*)ctx->rcx;
        ID3D11DepthStencilView* pDSV = (ID3D11DepthStencilView*)ctx->rdx;
        UINT ClearFlags = *(UINT*)(&ctx->r8);
        this->pGraphicsManager->OnClearDepthStencilView(pCtx, pDSV, ClearFlags);
        return MulNX::Hook::Then::Continue;
        }).value();
    this->RegisterAttachHook(this->hkClearDepthStencilView, "ClearDepthStencilView");
}
void HookD3D11::HookD3D11SwapChain(IDXGISwapChain* pSwapChain) {
    this->UpdateRenderXY(pSwapChain);
    using Microsoft::WRL::ComPtr;
    ComPtr<IDXGIDevice> dxgiDevice = nullptr;
    pSwapChain->GetDevice(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    dxgiDevice->QueryInterface(__uuidof(ID3D11Device), (void**)&this->pGraphicsManager->pd3dDevice);
    this->pGraphicsManager->pd3dDevice->GetImmediateContext(&this->pGraphicsManager->pd3dContext);

    this->HookD3D11DeviceAndContext();
    // 函数开头：
    // 0~4：Steam钩子（OBS游戏捕获钩子会与其交互，进行画面捕获）
    // 5~9：在这里部署MulNX的钩子，注意此时OBS捕获已经完成，可以做到启动顺序无关的渲染分离
    // 10+：其它汇编指令，我们的MulNX钩子最终跳转继续执行
    this->hkPresent = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pSwapChain)->GetVFuncPtr(8) + 5, [this](MulNX::Hook* hk, RegContext* ctx) {
        this->pGraphicsManager->pSwapChain = std::bit_cast<IDXGISwapChain*>(ctx->rcx);
        this->PublishSync("Hook/Present/First"_hash);
        hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->D3D11AndImGuiInit(hk, ctx);});
        return MulNX::Hook::Then::Continue;
        }).value();
    this->RegisterAttachHook(this->hkPresent, "Present");

    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    MulNX::Message msg("Hook/hWnd"_hash);
    auto&& [hWnd] = msg.Access<HWND>();
    hWnd = sd.OutputWindow;
    this->hCS2Wnd = sd.OutputWindow;
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
        ImGui_ImplWin32_NewFrame();          // 更新键盘、时间等
        // 缩放修正
        ImGuiIO& io = ImGui::GetIO();
        DXGI_SWAP_CHAIN_DESC sd;
        if (this->pGraphicsManager->pSwapChain &&
            SUCCEEDED(this->pGraphicsManager->pSwapChain->GetDesc(&sd))) {
            io.DisplaySize = ImVec2((float)sd.BufferDesc.Width, (float)sd.BufferDesc.Height);
            io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        }
        // 开启新帧
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

    this->hkResizeBuffers = MulNX::Hook::Create(
        (uint8_t*)IVClass::Assume(std::bit_cast<IDXGISwapChain*>(ctx->rcx))->GetVFuncPtr(13), [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->HandleOnResizeBuffers(hk, ctx);
        }).value();
    this->RegisterAttachHook(this->hkResizeBuffers, "ResizeBuffers");

    return MulNX::Hook::Then::Continue;
}
MulNX::Hook::Then HookD3D11::HandleOnPresent(MulNX::Hook* hk, RegContext* ctx) {
    this->pGraphicsManager->pSwapChain = (IDXGISwapChain*)ctx->rcx;
    // 绿幕渲染
    this->pGraphicsManager->BuildNew();
    this->pGraphicsManager->OnPresent();
    // UI 系统渲染
    this->pUISystem->HandleUpdate();    // 处理消息（坐标已在 HookWindow 中预缩放）
    this->pUISystem->Render();
    return MulNX::Hook::Then::Continue;
}
MulNX::Hook::Then HookD3D11::HandleOnResizeBuffers(MulNX::Hook* hk, RegContext* ctx) {
    this->pGraphicsManager->ReleaseOld();
    this->pGraphicsManager->pSwapChain = std::bit_cast<IDXGISwapChain*>(ctx->rcx);
    this->PublishSync("Hook/IDXGISwapChain/ResizeBuffers/Pre"_hash);
    ImGui_ImplDX11_InvalidateDeviceObjects();
    hk->CallMaybeOrigin(2, ctx);
    if (!ImGui_ImplDX11_CreateDeviceObjects()) {
        MulNX::ErrorTerminate("在重置后台缓冲区触发的ImGui资源重建中遇到错误！");
    }
    this->UpdateRenderXY(std::bit_cast<IDXGISwapChain*>(ctx->rcx));
    this->PublishSync("Hook/IDXGISwapChain/ResizeBuffers/Post"_hash);
    return MulNX::Hook::Then::Return;
}