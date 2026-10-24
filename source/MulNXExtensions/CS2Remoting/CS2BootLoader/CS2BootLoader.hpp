#pragma once
#include <MulNX/MulNX.hpp>

class CS2BootLoader final : public MulNX::Module<CS2BootLoader> {
    class CS2HelperController* pHelperController = nullptr;

    fs::path gamePath;
    std::string launchOptions;

    std::vector<std::string> patternsCheckDangerous;
    bool environmentChecked = false;
    bool CheckEnvironment();
    bool LaunchAndInject(const std::function<void(const PROCESS_INFORMATION&)>& created = {});

    void Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& msg);
    bool Init()override;
public:
    // 仅供非交互入口使用，auto.yaml 和普通界面启动保持原行为。
    bool externalLaunch = false;
    bool LaunchChecked(std::atomic<const char*>& stage, const std::function<void(const PROCESS_INFORMATION&)>& created);
};