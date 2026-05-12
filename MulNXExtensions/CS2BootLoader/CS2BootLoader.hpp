#pragma once

#include <MulNX/MulNX.hpp>

class CS2BootLoader final : public MulNX::ModuleBase {
    std::filesystem::path gamePath;
    std::filesystem::path dllPath;   // CS2OBTool.dll 路径
    std::string launchOptions;

    bool LaunchAndInject();
    bool IsGameRunning();
    bool InjectDll(HANDLE hProcess);
    bool InitDLL(HANDLE hProcess);

    bool Window(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& msg);
public:
    bool Init()override;
    void Deinit()override;
};