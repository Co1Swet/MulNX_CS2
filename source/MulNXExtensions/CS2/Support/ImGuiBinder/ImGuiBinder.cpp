#include "ImGuiBinder.hpp"
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>

bool ImGuiBinder::Init() {
    this->pUISystem = this->FindModule<MulNX::UISystem>("UISystem");
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    this->SubscribeSync("GraphicsSync/D3D11/Init/Pre", [this](auto&&...) {
        this->ImGuiInit();
        });

    this->SubscribeSync("Hook/Present", [this](auto&&...) {
        // UI 系统渲染
        this->pUISystem->HandleUpdate();    // 处理消息（坐标已在 HookWindow 中预缩放）
        this->pUISystem->Render();
        });

    this->SubscribeSync("Hook/IDXGISwapChain/ResizeBuffers/Pre", [this](auto&&...) {
        ImGui_ImplDX11_InvalidateDeviceObjects();
        });
    
    this->SubscribeSync("Hook/IDXGISwapChain/ResizeBuffers/Post", [this](auto&&...) {
        if (!ImGui_ImplDX11_CreateDeviceObjects()) {
            MulNX::ErrorTerminate("在重置后台缓冲区触发的ImGui资源重建中遇到错误！");
        }
        });

    return true;
}

void ImGuiBinder::ImGuiInit() {
    // ImGui 初始化
    ImGui_ImplDX11_Init(this->pGraphicsManager->pd3dDevice, this->pGraphicsManager->pd3dContext);

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

        auto ctx = this->pGraphicsManager->pd3dContext;
        ComPtr<ID3D11RenderTargetView> savedRTV = nullptr;
        ComPtr<ID3D11DepthStencilView> savedDSV = nullptr;
        ctx->OMGetRenderTargets(1, &savedRTV, &savedDSV);

        ID3D11RenderTargetView* rtv = this->pGraphicsManager->refBackBufferView.Get();
        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        ID3D11RenderTargetView* rawRTV = savedRTV.Get();
        ctx->OMSetRenderTargets(1, &rawRTV, savedDSV.Get());
        };
}