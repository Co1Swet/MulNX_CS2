#include "HookWindow.hpp"
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <shellapi.h>

bool HookWindow::Init() {
    this->pUISystem = this->FindModule<MulNX::UISystem>("UISystem");

    this->SubscribeSync("Hook/hWnd", [this](MulNX::Message& msg) {
        auto&& [hWnd] = msg.Access<HWND>();
        this->hCS2Wnd = hWnd;
        // 窗口过程钩子
        this->hkWndProc = MulNX::Hook::Create((uint8_t*)GetWindowLongPtrW(this->hCS2Wnd, GWLP_WNDPROC), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto then = this->HandleWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
            if (then == MulNX::Hook::Then::Return)ctx->rax = 0;
            return then;
            }).value();
        this->RegisterAttachHook(this->hkWndProc, "WndProc");
        ImGui_ImplWin32_Init(this->hCS2Wnd);
        });
    return true;
}
void HookWindow::FixMouse(UINT& uMsg, LPARAM& lParam) {
    if (!MulNX::Win32::IsMouseMessage(uMsg)) return;
    RECT rc;
    if (!GetClientRect(this->hCS2Wnd, &rc)) return;
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    int newX = (int)(x * ((float)this->pGlobalVars->renderX.load(std::memory_order_acquire) / (rc.right - rc.left)));
    int newY = (int)(y * ((float)this->pGlobalVars->renderY.load(std::memory_order_acquire) / (rc.bottom - rc.top)));
    lParam = MAKELPARAM(newX, newY);
}
bool HookWindow::CheckIme(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto cur = this->pUISystem->WantTextInput.load();
    if (!this->lastWantTextInput && cur) {
        ImmAssociateContextEx(hWnd, NULL, IACE_DEFAULT);
    }
    if (this->lastWantTextInput && !cur) {
        ImmAssociateContextEx(hWnd, NULL, 0);
    }
    this->lastWantTextInput = cur;
    if (!MulNX::Win32::IsImeMessage(uMsg))return false;
    if (cur) {
        return true;
    }
    return false;
}
MulNX::Hook::Then HookWindow::HandleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_IME_CHAR) {
        uMsg = WM_CHAR;
    }
    auto bRet = this->CheckIme(hWnd, uMsg, wParam, lParam);
    this->FixMouse(uMsg, lParam);
    this->pUISystem->winMsgs.enqueue({ hWnd, uMsg, wParam, lParam });
    if (bRet)return MulNX::Hook::Then::Return;

    if (MulNX::Win32::IsMouseMessage(uMsg)) {
        if (this->pUISystem->WantCaptureMouse.load(std::memory_order_acquire)) {
            return MulNX::Hook::Then::Return;
        }
    }
    if (MulNX::Win32::IsKeyboardMessage(uMsg)) {
        if (this->pUISystem->WantTextInput.load(std::memory_order_acquire) ||
            this->pInputSystem->IsKeyPressed(VK_MENU)) {
            return MulNX::Hook::Then::Return; // 当alt按下时进行拦截，此时属于 MulNX 按键通道判定快捷键的时刻
        }
    }
    if (wParam == VK_TAB) {
        if (this->pUISystem->WantCaptureMouse.load(std::memory_order_acquire)) {
            return MulNX::Hook::Then::Return;
        }
    }
    if (uMsg == WM_CLOSE) {
        int result = MessageBoxW(hWnd,
            L"要关闭游戏前，必须先卸载 MulNX。\n点击“确定”卸载 MulNX，之后可再次尝试关闭游戏。",
            L"MulNX 警告",
            MB_OKCANCEL | MB_ICONWARNING | MB_TOPMOST
        );
        if (result == IDOK) {
            this->hkWndProc->Detach();
            this->LogWarning(I18n("sys.shutdown_warning"));
            this->Core->Driver()->smutex.unlock();
        }
        return MulNX::Hook::Then::Return;
    }
    return MulNX::Hook::Then::Continue;
}