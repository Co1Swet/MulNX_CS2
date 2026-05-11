#pragma once

#include <MulNX/MulNX.hpp>

class CS2BootLoader final : public MulNX::ModuleBase {
    std::filesystem::path gamePath;
    std::filesystem::path dllPath;   // CS2OBTool.dll 路径
    std::vector<std::string> launchOptions;

    bool LaunchAndInject();
    bool IsGameRunning();
    bool InjectDll(HANDLE hProcess, const std::wstring& dllPath);

    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
    void Deinit()override;
};