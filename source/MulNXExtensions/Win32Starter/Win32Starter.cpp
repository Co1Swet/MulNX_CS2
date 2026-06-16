#include "Win32Starter.hpp"
#pragma comment(lib,"d3d11.lib")

Win32Starter::Win32Starter() {
    this->pThis = this;
}

bool Win32Starter::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::UISystem>("UISystem");

    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    this->wc = { sizeof(wc), CS_CLASSDC, this->EntryWndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"MulNX Exe Example", nullptr };
    RegisterClassExW(&wc);
    this->hwnd = ::CreateWindowW(wc.lpszClassName, L"Multiple Next Extension", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(this->pd3dDevice, this->pd3dDeviceContext);

    this->clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    this->pUISystem->FrameBefore = [this]() {
        // Handle window being minimized or screen locked
        if (this->SwapChainOccluded && this->pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            return false;
        }
        this->SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (this->ResizeWidth != 0 && this->ResizeHeight != 0) {
            CleanupRenderTarget();
            this->pSwapChain->ResizeBuffers(0, this->ResizeWidth, this->ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            this->ResizeWidth = this->ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        return true;
        };

    this->pUISystem->FrameBehind = [this]() {
        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            this->clear_color.x * this->clear_color.w,
            this->clear_color.y * this->clear_color.w,
            this->clear_color.z * this->clear_color.w,
            this->clear_color.w
        };
        this->pd3dDeviceContext->OMSetRenderTargets(1, &this->mainRenderTargetView, nullptr);
        this->pd3dDeviceContext->ClearRenderTargetView(this->mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        // Present
        HRESULT hr = this->pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        this->SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
        };
    // 通过MainDraw字符串发送UI启动命令
    this->CreateMainDraw();

    // 设置系统标志位
    this->pGlobalVars->SystemReady.store(true);
    return true;
}
void Win32Starter::Run() {
    bool running = true;
    while (running) {
        MSG msg;
        this->pUISystem->HandleUpdate();
        this->pUISystem->Render();
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
        }
    }
    this->LogWarning(I18n("sys.shutdown_warning"));
    this->CloseSystem();
    this->Core->Close();
    return;
}
void Win32Starter::Deinit() {
    // UISystem 已析构，而该指针指向其所有的缓冲区，这里手动置空，防止UB
    ImGui::GetIO().IniFilename = nullptr;
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(this->hwnd);
    UnregisterClassW(this->wc.lpszClassName, this->wc.hInstance);
}

bool Win32Starter::CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &this->pSwapChain, &this->pd3dDevice, &featureLevel, &this->pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &this->pSwapChain, &this->pd3dDevice, &featureLevel, &this->pd3dDeviceContext);
    if (res != S_OK)
        return false;

    IDXGIFactory* pSwapChainFactory;
    if (SUCCEEDED(this->pSwapChain->GetParent(IID_PPV_ARGS(&pSwapChainFactory)))) {
        pSwapChainFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        pSwapChainFactory->Release();
    }

    CreateRenderTarget();
    return true;
}
void Win32Starter::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (this->pSwapChain) { this->pSwapChain->Release(); this->pSwapChain = nullptr; }
    if (this->pd3dDeviceContext) { this->pd3dDeviceContext->Release(); this->pd3dDeviceContext = nullptr; }
    if (this->pd3dDevice) { this->pd3dDevice->Release(); this->pd3dDevice = nullptr; }
}
void Win32Starter::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    this->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    this->pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &this->mainRenderTargetView);
    pBackBuffer->Release();
}
void Win32Starter::CleanupRenderTarget() {
    if (this->mainRenderTargetView) { this->mainRenderTargetView->Release(); this->mainRenderTargetView = nullptr; }
}
LRESULT WINAPI Win32Starter::EntryWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return Win32Starter::pThis->WndProc(hWnd, msg, wParam, lParam);
}
LRESULT WINAPI Win32Starter::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    this->pUISystem->winMsgs.enqueue({ hWnd,msg,wParam,lParam });

    if (this->pUISystem->WantCaptureMouse.load(std::memory_order_acquire) && MulNX::Win32::IsMouseMessage(msg))
        return 0;
    if (MulNX::Win32::IsKeyboardMessage(msg)) {
        if (this->pUISystem->WantTextInput.load(std::memory_order_acquire) || this->pInputSystem->IsKeyPressed(VK_MENU))
            return 0; // 当alt按下时进行拦截，此时属于 MulNX 按键通道判定快捷键的时刻
    }

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        this->ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        this->ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}