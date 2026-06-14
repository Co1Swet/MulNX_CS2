#include "DemoAnalyzer.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>

bool DemoAnalyzer::Init() {
    auto toolsPath = this->ISys().Path()->PathGetForShared("Tools");
    this->csdaPath = toolsPath / "csda.exe";
    this->dirData = this->ISys().Path()->PathGetForShared("Data");

    this->ISys().SubscribeAsync("Demo/Analyze");

    this->ISys().SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemoAnalyzer::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Analyze"_hash: {
        std::string demoPath = msg.asp.get<MulNX::NetExt>()->str1;
        this->ISys().LogInfo("Received demo path: " + demoPath);
        this->AnalyzeDemoWithCSDA(std::move(demoPath)).resume();
        break;
    }
    default:
        break;
    }
}

MulNX::CoTask DemoAnalyzer::AnalyzeDemoWithCSDA(std::filesystem::path&& pthDemo) {
    // ---------- 调用 csda.exe ----------
    // 1. 构建命令行字符串，注意路径带引号，以防空格
    std::ostringstream cmdLine;
    cmdLine << "\"" << this->csdaPath.string() << "\" "
        << "-demo-path=" << pthDemo << " "
        << "-output=\"" << this->dirData.string() << "\" "
        << "-format=json";   // 可根据需要加 -positions 等

    std::string cmdStr = cmdLine.str();
    this->ISys().LogInfo("Executing: " + cmdStr);

    // 准备 STARTUPINFO 和 PROCESS_INFORMATION
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 注意：需要将 cmdStr 复制到可修改的缓冲区，因为 CreateProcess 可能会修改它
    auto cmdBuffer = MulNX::CharUtility::U8ToW(cmdStr);

    // 创建进程
    BOOL success = CreateProcessW(
        nullptr,               // 应用程序名
        cmdBuffer.data(),      // 命令行（注意是可修改的）
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success) {
        DWORD err = GetLastError();
        this->ISys().LogError("Failed to create process, error code: " + std::to_string(err));
        co_return;
    }

    co_await this->WaitUntil([hProcess = pi.hProcess] {
        return WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0;
        });

    // 获取退出码并检查
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    if (exitCode != 0) {
        this->ISys().LogError("csda.exe exited with code: " + std::to_string(exitCode));
    }
    else {
        this->ISys().LogInfo("Demo analysis completed successfully.");
    }

    // 清理句柄
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}