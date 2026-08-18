#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXUtils/WinExt/HookMixin.hpp>
#include <oleidl.h>

class HookWindow final : public MulNX::Module<HookWindow>, public HookMixin<HookWindow> {
    MulNX::UISystem* pUISystem = nullptr;
    HWND hCS2Wnd = nullptr;
    // 窗口过程钩子
    std::unique_ptr<MulNX::Hook> hkWndProc = nullptr;
    bool lastWantTextInput = false;
    bool CheckIme(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void FixMouse(UINT& uMsg, LPARAM& lParam);
    MulNX::Hook::Then HandleWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // 文件拖拽钩子
    std::unique_ptr<MulNX::Hook> hkDrop = nullptr;
    void HandleProcessDropFiles(IDataObject* pDataObj);

    bool Init()override;
};