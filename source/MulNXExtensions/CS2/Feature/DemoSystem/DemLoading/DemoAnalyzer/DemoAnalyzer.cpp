#include "DemoAnalyzer.hpp"
#include <MulNX/Common/Misc.hpp>
#include <MulNX/Base/CharUtility/CharUtility.hpp>

namespace csda {
    inline constexpr std::array<std::string_view, 11> kDemoSources = {
        "challengermode",
        "ebot",
        "esea",
        "esl",
        "esportal",
        "faceit",
        "fastcup",
        "5eplay",
        "perfectworld",
        "popflash",
        "valve",
    };
}

bool DemoAnalyzer::Init() {
    auto toolsPath = this->Path()->PathGetForShared("Tools");
    this->csdaPath = toolsPath / "csda.exe";
    this->dirData = this->Path()->PathGetForShared("Data");

    (*this)
        .SubscribeAsyncEx<void>("Demo/Analyze")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemoAnalyzer::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Analyze"_hash: {
        const std::string& str = msg.asp.get<MulNX::NetExt>()->str1;
        fs::path demoPath(str);
        // 确保为绝对路径（正常情况下已经是）
        if (!demoPath.is_absolute()) {
            this->LogError(std::format("收到非绝对路径: {}", str));
            break;
        }
        this->LogInfo(std::format("接收到分析请求: {}", demoPath.string()));
        this->HandleAnalyzeRequest(std::move(demoPath)).Fire();
        break;
    }
    default:
        break;
    }
}

MulNX::CoTask DemoAnalyzer::HandleAnalyzeRequest(fs::path&& demoPath) {
    std::string stem = demoPath.stem().string();
    fs::path jsonPath = this->dirData / (stem + ".json");

    // JSON 已存在 → 直接发送加载消息
    if (fs::exists(jsonPath)) {
        this->LogInfo(std::format("分析结果已存在，直接加载: {}", demoPath.string()));
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/JSON/Read"_hash);
        rp->str1 = stem;
        this->PublishAsync(std::move(msg));
        co_return;
    }

    // 检查是否正在分析
    {
        std::unique_lock lock(this->smutex);
        if (this->analyzingSet.find(demoPath) != this->analyzingSet.end()) {
            this->LogWarning(std::format("该 Demo 正在分析中: {}", demoPath.string()));
            co_return;
        }
        else {
            this->analyzingSet.insert(demoPath);
        }
    }

    this->LogInfo(std::format("开始分析: {}", demoPath.string()));
    co_await this->AnalyzeDemoWithCSDA(std::move(demoPath));
}

MulNX::CoTask DemoAnalyzer::AnalyzeDemoWithCSDA(fs::path&& demoPath) {
    std::string stem = demoPath.stem().string();

    // RAII：分析结束时从队列移除
    auto cleanup = scope_exit([&] {
        std::unique_lock lock(this->smutex);
        this->analyzingSet.erase(demoPath);
        });

    for (int i = -1;i < std::ssize(csda::kDemoSources);++i) {
        std::string cmdStr = std::format(
            "\"{}\" -demo-path=\"{}\" -output=\"{}\" -format=json",
            this->csdaPath.string(), demoPath.string(), this->dirData.string());

        if (i >= 0) {
            cmdStr += std::format(" -source={}",
                csda::kDemoSources[static_cast<std::size_t>(i)]);
        }

        this->LogInfo(std::format("尝试执行: {}", cmdStr));

        STARTUPINFOW si{ sizeof(si) };
        PROCESS_INFORMATION pi{};
        auto cmdBuffer = MulNX::CharUtility::U8ToW(cmdStr);

        if (!CreateProcessW(nullptr, cmdBuffer.data(), nullptr, nullptr,
            FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            this->LogError(std::format("创建进程失败，错误码: {}", GetLastError()));
            co_return;   // cleanup 自动执行，不发送消息
        }
        auto onExit = scope_exit([&] {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            });

        co_await this->WaitUntil([&] {
            return WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0;
            });

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        if (exitCode == 0) {
            this->LogInfo("Demo 分析成功完成");
            // 仅成功时发送加载消息
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/JSON/Read"_hash);
            rp->str1 = stem;
            this->PublishAsync(std::move(msg));
            co_return;
        }
        this->LogWarning(std::format("csda.exe 退出码: {}，正在加路径反复尝试", exitCode));
    }
    this->LogError("所有尝试均失败，无法分析");
    // cleanup 析构，移除队列
}