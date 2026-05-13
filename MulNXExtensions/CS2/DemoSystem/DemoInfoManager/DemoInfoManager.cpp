#include "DemoInfoManager.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>

bool DemoInfoManager::Init() {
    auto toolsPath = this->ISys().PathManager()->PathGetForShared("Tools");
    this->csdaPath = toolsPath / "csda.exe";
    this->outputDir = toolsPath / "Output";

    this->ISys().SubscribeAsync("Demo/Analyze");

    this->SendTask("DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemoInfoManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Analyze"_hash: {
        std::string demoPath = msg.asp.get<MulNX::NetExt>()->str1;
        this->ISys().LogInfo("Received demo path: " + demoPath);
        this->currentDemoPath = demoPath;
        this->AnalyzeDemoWithCSDA();
        break;
    }
    default:
        break;
    }
}

void DemoInfoManager::AnalyzeDemoWithCSDA() {
    // ---------- 调用 csda.exe ----------
    // 1. 构建命令行字符串，注意路径带引号，以防空格
    std::ostringstream cmdLine;
    cmdLine << "\"" << this->csdaPath.string() << "\" "
        << "-demo-path=" << this->currentDemoPath << " "
        << "-output=\"" << this->outputDir.string() << "\" "
        << "-format=json";   // 可根据需要加 -positions 等

    std::string cmdStr = cmdLine.str();
    this->ISys().LogInfo("Executing: " + cmdStr);

    // 2. 准备 STARTUPINFO 和 PROCESS_INFORMATION
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 使用 CREATE_NO_WINDOW 避免弹出黑窗口
    DWORD creationFlags = CREATE_NO_WINDOW;

    // 注意：需要将 cmdStr 复制到可修改的缓冲区，因为 CreateProcess 可能会修改它
    auto cmdBuffer = MulNX::CharUtility::U8ToW(cmdStr);

    // 3. 创建进程
    BOOL success = CreateProcessW(
        nullptr,               // 应用程序名
        cmdBuffer.data(),      // 命令行（注意是可修改的）
        nullptr,
        nullptr,
        FALSE,
        creationFlags,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success) {
        DWORD err = GetLastError();
        this->ISys().LogError("Failed to create process, error code: " + std::to_string(err));
        return;
    }

    // 4. 等待进程结束（可设置超时，这里无限等待）
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 5. 获取退出码并检查
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    if (exitCode != 0) {
        this->ISys().LogError("csda.exe exited with code: " + std::to_string(exitCode));
    }
    else {
        this->ISys().LogInfo("Demo analysis completed successfully.");
        // 可选：解析输出 JSON 文件
        // std::filesystem::path jsonFile = this->outputDir / "111.json";
        // ...
    }

    // 6. 清理句柄
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}