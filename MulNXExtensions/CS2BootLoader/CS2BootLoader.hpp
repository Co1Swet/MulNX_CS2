#pragma once

#include <MulNX/MulNX.hpp>

class DLLInjectHelper;
class CS2BootLoader final : public MulNX::ModuleBase {
    DLLInjectHelper* pInjectHelper = nullptr;

    std::filesystem::path gamePath;
    std::filesystem::path dllPath;   // CS2OBTool.dll 路径
    std::filesystem::path helperPath;
    std::filesystem::path dirFfmpeg;   
    std::string launchOptions;

    bool LaunchAndInject();
    bool IsGameRunning();

    bool Window(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& msg);
public:
    bool Init()override;
    void Deinit()override;
};