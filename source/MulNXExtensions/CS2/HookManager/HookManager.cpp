#include "HookManager.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <shellapi.h>

bool HookManager::Init() {
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
void HookManager::HookD3D11DeviceAndContext() {
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
void HookManager::HookD3D11SwapChain(IDXGISwapChain* pSwapChain) {
    // Hook Present函数
    // 函数开头：
    // 0~4：Steam钩子（OBS游戏捕获钩子会与其交互，进行画面捕获）
    // 5~9：在这里部署MulNX的钩子，注意此时OBS捕获已经完成，可以做到启动顺序无关的渲染分离
    // 10+：其它汇编指令，我们的MulNX钩子最终跳转继续执行
    this->hkPresent = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pSwapChain)->GetVFuncPtr(8) + 5, [this](MulNX::Hook* hk, RegContext* ctx) {
        this->PublishSync("Hook/Present/Fisrt"_hash);
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
    this->CS2hWnd = sd.OutputWindow;
    // 文件拖拽钩子
    HANDLE hProp = GetPropW(this->CS2hWnd, L"OleDropTargetInterface");
    IDropTarget* pTarget = static_cast<IDropTarget*>(hProp);
    this->hkDrop = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTarget)->GetVFuncPtr(6), [this](MulNX::Hook* hk, RegContext* ctx) {
        this->HandleProcessDropFiles((IDataObject*)ctx->rdx);
        return MulNX::Hook::Then::Continue;
        }).value();
    this->hkDrop->Attach();
    this->LogSucc(I18n("hook.attached", "OleDropTargetInterface::Drop"));
    // 窗口过程钩子
    this->hkWndProc = MulNX::Hook::Create((uint8_t*)GetWindowLongPtrW(this->CS2hWnd, GWLP_WNDPROC), [this](MulNX::Hook* hk, RegContext* ctx) {
        return this->HandleWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
        }).value();
    this->hkWndProc->Attach();
    this->LogSucc(I18n("hook.attached", "WndProc"));
}
MulNX::Hook::Then HookManager::D3D11AndImGuiInit(MulNX::Hook* hk, RegContext* ctx) {
    hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->HandleOnPresent(hk, ctx);});
    // ImGui 初始化
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(this->CS2hWnd);
    ImGui_ImplDX11_Init(this->pGraphicsManager->pd3dDevice, this->pGraphicsManager->pd3dContext);
    // 创建绿幕着色器资源
    this->pGraphicsManager->ReleaseOld();
    this->pGraphicsManager->CreateGreenScreenAssets();

    this->pUISystem->FrameBefore = [this]() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::GetBackgroundDrawList()->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
            static_cast<HookManager*>(cmd->UserCallbackData)->PublishSync("Hook/BeforePresent"_hash);
            }, this, 0);
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
MulNX::Hook::Then HookManager::HandleOnPresent(MulNX::Hook* hk, RegContext* ctx) {
    this->pGraphicsManager->pSwapChain = (IDXGISwapChain*)ctx->rcx;
    // 绿幕渲染
    this->pGraphicsManager->BuildNew();
    this->pGraphicsManager->OnPresent();
    // UI 系统渲染
    this->pUISystem->HandleUpdate();
    this->pUISystem->Render();
    return MulNX::Hook::Then::Continue;
}
MulNX::Hook::Then HookManager::HandleWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    this->pUISystem->winMsgs.enqueue({ hwnd, uMsg, wParam, lParam });
    if (this->pUISystem->WantCaptureMouse.load(std::memory_order_acquire) && MulNX::Win32::IsMouseMessage(uMsg))
        return MulNX::Hook::Then::Return;
    if (MulNX::Win32::IsKeyboardMessage(uMsg)) {
        if (this->pUISystem->WantTextInput.load(std::memory_order_acquire) || this->pInputSystem->IsKeyPressed(VK_MENU))
            return MulNX::Hook::Then::Return; // 当alt按下时进行拦截，此时属于 MulNX 按键通道判定快捷键的时刻
    }
    if (uMsg == WM_CLOSE) {
        this->LogWarning(I18n("sys.shutdown_warning"));
        this->Core->Driver()->CloseSystem();
    }
    return MulNX::Hook::Then::Continue;
}
void HookManager::Deinit() {
    this->hkClearDepthStencilView->Detach();
    this->hkPresent->Detach();
    this->hkWndProc->Detach();
}
void HookManager::HandleProcessDropFiles(IDataObject* pDataObj) {
    if (!pDataObj) return;
    // 请求 CF_HDROP 格式
    FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM med{};
    if (FAILED(pDataObj->GetData(&fmt, &med))) return;
    // 锁住全局内存，拿到 HDROP
    HDROP hDrop = static_cast<HDROP>(GlobalLock(med.hGlobal));
    if (!hDrop) {
        ReleaseStgMedium(&med);
        return;
    }
    UINT numFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    // try catch保证无论如何，占有的句柄必须释放回去
    try {
        for (UINT i = 0; i < numFiles; ++i) {
            UINT len = DragQueryFileW(hDrop, i, nullptr, 0);  // 含 '\0'
            if (len == 0) continue;
            std::wstring buffer(len, L'\0');
            if (!DragQueryFileW(hDrop, i, buffer.data(), len + 1))continue;
            std::filesystem::path filePath{ buffer };
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Window/Drag/FileDrop"_hash);
            rp->str1 = std::move(filePath.string());
            this->PublishAsync(std::move(msg));
        }
    }
    catch (const std::exception& e) {
        this->LogError(I18n("win32.drag.analisy.error", e.what()));
    }
    catch (...) {
        this->LogError(I18n("win32.drag.analisy.unk_error"));
    }
    GlobalUnlock(med.hGlobal);
    ReleaseStgMedium(&med);
}