#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketMixin.hpp>

class CS2BootLoader final : public MulNX::Module<CS2BootLoader>, public WebSocketMixin<CS2BootLoader> {
    class CS2HelperController* pHelperController = nullptr;

    fs::path gamePath;
    std::string launchOptions;

    struct LaunchResult {
        std::string requestId;
        bool ok = false;
        std::string stage = "environment";
        DWORD pid = 0;
        std::string created;
        DWORD errorCode = 0;
        std::string message;
        bool cleanupPending = false;
        DWORD cleanupErrorCode = 0;
    };
    LaunchResult lastResult;
    uint64_t highestRequestId = 0;
    PROCESS_INFORMATION game{};
    std::string gameCreated;
    bool gameResumed = false;
    void PostResult(const LaunchResult& result);
    void CloseGame();
    bool CleanupGame(DWORD& errorCode);

    std::vector<std::string> patternsCheckDangerous;
    bool environmentChecked = false;
    bool CheckEnvironment(LaunchResult& result);
    bool LaunchAndInject(LaunchResult& result);

    void Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& msg);
    bool Init()override;
public:
    ~CS2BootLoader();
};