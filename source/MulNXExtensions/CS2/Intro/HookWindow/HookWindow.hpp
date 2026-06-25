#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>

class HookWindow final : public CSModuleBase {
private:
    MulNX::UISystem* pUISystem = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    HWND hCS2Wnd = nullptr;
    // 窗口过程钩子
    std::unique_ptr<MulNX::Hook> hkWndProc = nullptr;
    void FixMouse(UINT& uMsg, LPARAM& lParam);
    MulNX::Hook::Then HandleWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // 文件拖拽钩子
    std::unique_ptr<MulNX::Hook> hkDrop = nullptr;
    void HandleProcessDropFiles(IDataObject* pDataObj);
    bool Init()override;
    void Deinit()override;
};