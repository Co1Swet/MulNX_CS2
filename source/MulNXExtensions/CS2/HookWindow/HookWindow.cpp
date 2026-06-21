#include "HookWindow.hpp"
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <shellapi.h>

bool HookWindow::Init() {
    this->pUISystem = this->FindModule<MulNX::UISystem>("UISystem");
    this->SubscribeSync("Hook/hWnd", [this](MulNX::Message& msg) {
        this->hCS2Wnd = msg.p1.as<HWND>();
        // 文件拖拽钩子
        HANDLE hProp = GetPropW(this->hCS2Wnd, L"OleDropTargetInterface");
        IDropTarget* pTarget = static_cast<IDropTarget*>(hProp);
        this->hkDrop = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTarget)->GetVFuncPtr(6), [this](MulNX::Hook* hk, RegContext* ctx) {
            this->HandleProcessDropFiles((IDataObject*)ctx->rdx);
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkDrop->Attach();
        this->LogSucc(I18n("hook.attached", "OleDropTargetInterface::Drop"));
        // 窗口过程钩子
        this->hkWndProc = MulNX::Hook::Create((uint8_t*)GetWindowLongPtrW(this->hCS2Wnd, GWLP_WNDPROC), [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->HandleWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
            }).value();
        this->hkWndProc->Attach();
        this->LogSucc(I18n("hook.attached", "WndProc"));
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(this->hCS2Wnd);
        });
    return true;
}
void HookWindow::Deinit() {
    this->hkWndProc->Detach();
}
MulNX::Hook::Then HookWindow::HandleWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
void HookWindow::HandleProcessDropFiles(IDataObject* pDataObj) {
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