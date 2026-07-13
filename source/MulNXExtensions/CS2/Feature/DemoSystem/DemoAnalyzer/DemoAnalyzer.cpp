#include "DemoAnalyzer.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>

bool DemoAnalyzer::Init() {
    auto toolsPath = this->Path()->PathGetForShared("Tools");
    this->csdaPath = toolsPath / "csda.exe";
    this->dirData = this->Path()->PathGetForShared("Data");

    this->SubscribeAsync("Demo/Analyze");

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemoAnalyzer::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Analyze"_hash: {
        std::string str = msg.asp.get<MulNX::NetExt>()->str1;
        std::filesystem::path demoPath(str);
        // 确保为绝对路径（正常情况下已经是）
        if (!demoPath.is_absolute()) {
            this->LogWarning("收到非绝对路径: " + str);
            demoPath = std::filesystem::absolute(demoPath);
        }
        this->LogInfo("Received demo path: " + demoPath.string());
        this->HandleAnalyzeRequest(std::move(demoPath));
        break;
    }
    default:
        break;
    }
}

void DemoAnalyzer::HandleAnalyzeRequest(std::filesystem::path demoPath) {
    std::string stem = demoPath.stem().string();
    std::filesystem::path jsonPath = this->dirData / (stem + ".json");

    // 1. JSON 已存在 → 直接发送加载消息
    if (std::filesystem::exists(jsonPath)) {
        this->LogInfo("分析结果已存在，直接加载: " + demoPath.string());
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/JSON/Load"_hash);
        rp->str1 = stem;
        this->PublishAsync(std::move(msg));
        return;
    }

    // 2. 检查是否正在分析
    {
        std::unique_lock lock(this->smutex);
        if (this->analyzingSet.find(demoPath) != this->analyzingSet.end()) {
            this->LogWarning("该 Demo 正在分析中: " + demoPath.string());
            return;
        }
        else {
            this->analyzingSet.insert(demoPath);
        }
    }

    this->LogInfo("开始分析: " + demoPath.string());
    this->AnalyzeDemoWithCSDA(std::move(demoPath)).resume();
}

MulNX::CoTask DemoAnalyzer::AnalyzeDemoWithCSDA(std::filesystem::path demoPath) {
    std::string stem = demoPath.stem().string();

    // RAII：分析结束时从队列移除
    auto cleanup = scope_exit([this, demoPath] {
        std::unique_lock lock(this->smutex);
        analyzingSet.erase(demoPath);
        });

    // 构造 csda 命令行，确保路径包含引号以处理空格
    std::string cmdStr = "\"" + this->csdaPath.string() + "\" "
        + "-demo-path=\"" + demoPath.string() + "\" "
        + "-output=\"" + this->dirData.string() + "\" "
        + "-format=json";

    this->LogInfo("执行: " + cmdStr);

    STARTUPINFOW si{ sizeof(si) };
    PROCESS_INFORMATION pi{};
    auto cmdBuffer = MulNX::CharUtility::U8ToW(cmdStr);

    if (!CreateProcessW(nullptr, cmdBuffer.data(), nullptr, nullptr,
        FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        this->LogError("创建进程失败，错误码: " + std::to_string(GetLastError()));
        co_return;   // cleanup 自动执行，不发送消息
    }

    co_await this->WaitUntil([h = pi.hProcess] {
        return WaitForSingleObject(h, 0) == WAIT_OBJECT_0;
        });

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) {
        this->LogError("csda.exe 退出码: " + std::to_string(exitCode));
    }
    else {
        this->LogInfo("Demo 分析成功完成。");
        // 仅成功时发送加载消息
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/JSON/Load"_hash);
        rp->str1 = stem;
        this->PublishAsync(std::move(msg));
    }
    // cleanup 析构，移除队列
}