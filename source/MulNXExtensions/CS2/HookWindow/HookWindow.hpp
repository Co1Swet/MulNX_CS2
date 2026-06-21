#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <objidl.h>

class HookWindow final : public CSModuleBase {
private:
    MulNX::UISystem* pUISystem = nullptr;
    HWND hCS2Wnd = nullptr;
    // 窗口过程钩子
    std::unique_ptr<MulNX::Hook> hkWndProc = nullptr;
    MulNX::Hook::Then HandleWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // 文件拖拽钩子
    std::unique_ptr<MulNX::Hook> hkDrop = nullptr;
    void HandleProcessDropFiles(IDataObject* pDataObj);
public:
    bool Init()override;
    void Deinit()override;
};