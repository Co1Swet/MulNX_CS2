#pragma once
#include <MulNX/MulNX.hpp>

class CS2BootLoader final : public MulNX::Module<CS2BootLoader> {
    class CS2HelperController* pHelperController = nullptr;

    std::filesystem::path gamePath;
    std::string launchOptions;

    std::vector<std::string> patternsCheckDangerous;
    bool environmentChecked = false;
    bool CheckEnvironment();
    bool LaunchAndInject();
    bool IsGameRunning();

    bool Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& msg);
    bool Init()override;
    void Deinit()override;
};