#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/imgui_d11/imgui.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <d3d11.h>

class Win32Starter final :public MulNX::Core::CoreStarterBase {
    MulNX::UISystem* pUISystem = nullptr;

    // Data
    ID3D11Device* pd3dDevice = nullptr;
    ID3D11DeviceContext* pd3dDeviceContext = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    bool                     SwapChainOccluded = false;
    UINT                     ResizeWidth = 0, ResizeHeight = 0;
    ID3D11RenderTargetView* mainRenderTargetView = nullptr;

    WNDCLASSEXW wc;
    HWND hwnd;

    ImVec4 clear_color;

    // Forward declarations of helper functions
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    static LRESULT WINAPI EntryWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    inline static Win32Starter* pThis = nullptr;
public:
    Win32Starter();
    bool Init()override;
    void Deinit()override;

    void Run();
};