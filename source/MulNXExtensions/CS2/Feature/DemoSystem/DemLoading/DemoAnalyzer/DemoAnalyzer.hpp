#pragma once
#include <Intro/CSModuleBase.hpp>
#include <mutex>

class DemoAnalyzer final : public CSModuleBase {
    std::mutex updateMutex;
    bool stopping = false;

    fs::path csdaPath;
    fs::path dirData;
    std::set<fs::path> analyzingSet;            // 正在分析的 demo 绝对路径

    void HandleAnalyzeRequest(fs::path demoPath);
    MulNX::CoTask AnalyzeDemoWithCSDA(fs::path demoPath);

    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};