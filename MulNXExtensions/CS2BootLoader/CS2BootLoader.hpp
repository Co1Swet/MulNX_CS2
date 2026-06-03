#pragma once

#include <MulNX/MulNX.hpp>

class DLLInjectHelper;
class CS2BootLoader final : public MulNX::ModuleBase {
    DLLInjectHelper* pInjectHelper = nullptr;

    std::filesystem::path gamePath;
    std::filesystem::path helperPath;
    std::string launchOptions;

    std::vector<std::string> patternsCheckDangerous;
    bool environmentChecked = false;
    bool CheckEnvironment();
    bool LaunchAndInject();
    bool IsGameRunning();

    bool Window(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& msg);
public:
    bool Init()override;
    void Deinit()override;
};