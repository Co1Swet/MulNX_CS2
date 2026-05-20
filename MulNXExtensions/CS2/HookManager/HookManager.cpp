#include "HookManager.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <shellapi.h>
#pragma comment(lib, "d3d11.lib")

using CreateDXGIFactory1_t = HRESULT(WINAPI*)(void* riid, void** ppFactory);
using D3D11CreateDevice_t = HRESULT(WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
using CreateSwapChain_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);
using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

bool HookManager::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::UISystem>("UISystem");
    this->pGraphicsManager = this->Core->ModuleManager()->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    this->hkLoadLibraryExW = MulNX::Hook::Create((uint8_t*)LoadLibraryExW, [this](MulNX::Hook* hk, RegContext* ctx) {

        LPCWSTR lpLibFileName = (LPCWSTR)ctx->rcx;
        HANDLE hFile = (HANDLE)ctx->rdx;
        DWORD dwFlags = *reinterpret_cast<DWORD*>(&ctx->r8);

        auto result = reinterpret_cast<decltype(LoadLibraryExW)*>(hk->pMaybeRawFunc)(lpLibFileName, hFile, dwFlags);
        *reinterpret_cast<HMODULE*>(&ctx->rax) = result;

        MulNX::Message msg("Hook/LoadLibraryExW"_hash);
        msg.p1.as<LPCWSTR>() = lpLibFileName;
        this->ISys().PublishSync(msg);

        return MulNX::Hook::Then::Return;
        }).value();
    this->hkLoadLibraryExW->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "LoadLibraryExW"));

    this->ISys().SubscribeSync("Hook/LoadLibraryExW/d3d11.dll", [this](MulNX::Message& msg) {
        auto pD3D11CreateDevice = (uint8_t*)GetProcAddress(GetModuleHandleW(L"d3d11.dll"), "D3D11CreateDevice");

        this->hkD3D11CreateDevice = MulNX::Hook::Create(pD3D11CreateDevice, [this](MulNX::Hook* hk, RegContext* ctx) {
            this->hkD3D11CreateDevice->Detach();
            auto ppDevice = *hk->GetStackParam<ID3D11Device**>(ctx, 7);
            auto ppImmediateContext = *hk->GetStackParam<ID3D11DeviceContext**>(ctx, 9);

            *reinterpret_cast<HRESULT*>(&ctx->rax) = hk->CallMaybeOrigin(6, ctx);

            ID3D11Device* pDevice = *ppDevice;  // 假设调用成功后有效
            IDXGIDevice* pDXGIDevice = nullptr;
            pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
            if (pDXGIDevice) {
                IDXGIAdapter* pAdapter = nullptr;
                pDXGIDevice->GetAdapter(&pAdapter);
                pDXGIDevice->Release();

                if (pAdapter) {
                    IDXGIFactory* pFactory = nullptr;
                    pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pFactory);
                    pAdapter->Release();
                }
            }

            // ---- Hook ClearDepthStencilView (vtable index 53) ----
            this->hkClearDepthStencilView = MulNX::Hook::Create((uint8_t*)IVClass::Assume(*ppImmediateContext)->GetVFuncPtr(53), [this](MulNX::Hook* hk, RegContext* ctx) {
                ID3D11DeviceContext* pCtx = (ID3D11DeviceContext*)ctx->rcx;
                ID3D11DepthStencilView* pDSV = (ID3D11DepthStencilView*)ctx->rdx;
                UINT ClearFlags = *(UINT*)(&ctx->r8);
                this->pGraphicsManager->OnClearDepthStencilView(pCtx, pDSV, ClearFlags);
                return MulNX::Hook::Then::Continue;
                }).value();
            this->hkClearDepthStencilView->Attach();
            this->ISys().LogSucc(I18n("hook.attached", "ClearDepthStencilView"));

            return MulNX::Hook::Then::Return;
            }).value();
        this->hkD3D11CreateDevice->Attach();
        this->ISys().LogSucc(I18n("hook.attached", "D3D11CreateDevice"));
        this->SendTask("MulNXMain", [this]() {
            if (this->pInputSystem->IsKeyPressed(VK_INSERT)) {
                this->CreateHook();
                return false;
            }
            return true;
            });

        return;
        });

    return true;
}

void HookManager::CreateHook() {
    // 临时 D3D11 设备/交换链
    ID3D11Device* pTempD3DDevice = nullptr;
    IDXGISwapChain* pTempSwapChain = nullptr;
    ID3D11DeviceContext* pTempContext = nullptr;

    const unsigned level_count = 2;
    D3D_FEATURE_LEVEL levels[level_count] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetForegroundWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    HRESULT hResult = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, level_count, D3D11_SDK_VERSION, &sd,
        &pTempSwapChain, &pTempD3DDevice, nullptr, &pTempContext);
    if (FAILED(hResult))
        MulNX::ErrorTerminate("无法创建D3D11设备和交换链，错误代码: " + std::to_string(hResult));

    // Hook Present函数
    // 函数开头：
    // 0~4：Steam钩子（OBS游戏捕获钩子会与其交互，进行画面捕获）
    // 5~9：在这里部署MulNX的钩子，注意此时OBS捕获已经完成，可以做到启动顺序无关的渲染分离
    // 10+：其它汇编指令，我们的MulNX钩子最终跳转继续执行
    this->hkPresent = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(8) + 5,
        [this](MulNX::Hook* hk, RegContext* ctx) {
            if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                this->pGraphicsManager->pSwapChain = (IDXGISwapChain*)ctx->rcx;
                this->d3dInit();
                this->pGraphicsManager->BuildNew();
                this->pGraphicsManager->OnPresent();
                // UI 系统渲染
                this->pUISystem->HandleUpdate();
                this->pUISystem->Render();
            }
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkPresent->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "Present"));

    // ---- Hook ResizeBuffers (vtable index 13) ----
    this->hkResizeBuffers = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(13),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            this->pGraphicsManager->ReleaseOld();
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkResizeBuffers->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "ResizeBuffers"));

    pTempSwapChain->Release();
    pTempContext->Release();
    pTempD3DDevice->Release();

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
}

void HookManager::d3dInit() {
    if (this->d3dInited) return;
    this->d3dInited = true;

    this->pGraphicsManager->pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&this->pGraphicsManager->pd3dDevice);
    this->pGraphicsManager->pd3dDevice->GetImmediateContext(&this->pGraphicsManager->pd3dContext);

    DXGI_SWAP_CHAIN_DESC sd;
    this->pGraphicsManager->pSwapChain->GetDesc(&sd);
    this->CS2hWnd = sd.OutputWindow;
    // 文件拖拽钩子
    HANDLE hProp = GetPropW(this->CS2hWnd, L"OleDropTargetInterface");
    IDropTarget* pTarget = static_cast<IDropTarget*>(hProp);
    this->hkDrop = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTarget)->GetVFuncPtr(6),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            this->HandleProcessDropFiles((IDataObject*)ctx->rdx);
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkDrop->Attach();
    // 窗口过程钩子
    this->hkWndProc = MulNX::Hook::Create((uint8_t*)GetWindowLongPtrW(this->CS2hWnd, GWLP_WNDPROC),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->HandleWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
        }).value();
    this->hkWndProc->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "WndProc"));
    // 交换链 RTV
    ID3D11Texture2D* buf = nullptr;
    this->pGraphicsManager->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
    this->pGraphicsManager->pd3dDevice->CreateRenderTargetView(buf, nullptr, &this->pGraphicsManager->view);
    buf->Release();

    // ImGui 初始化
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(this->CS2hWnd);
    ImGui_ImplDX11_Init(this->pGraphicsManager->pd3dDevice, this->pGraphicsManager->pd3dContext);

    // 创建绿幕着色器资源
    this->pGraphicsManager->CreateGreenScreenAssets();

    this->CreateMainDraw();
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
        this->ISys().LogWarning(I18n("sys.shutdown_warning"));
        this->CloseSystem();
    }
    return MulNX::Hook::Then::Continue;
}

void HookManager::Deinit() {
    this->hkLoadLibraryExW->Detach();
    this->hkClearDepthStencilView->Detach();
    this->hkDrop->Detach();
    this->hkPresent->Detach();
    this->hkResizeBuffers->Detach();
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
            this->ISys().PublishAsync(std::move(msg));
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogError(I18n("win32.drag.analisy.error", e.what()));
    }
    catch (...) {
        this->ISys().LogError(I18n("win32.drag.analisy.unk_error"));
    }
    GlobalUnlock(med.hGlobal);
    ReleaseStgMedium(&med);
}