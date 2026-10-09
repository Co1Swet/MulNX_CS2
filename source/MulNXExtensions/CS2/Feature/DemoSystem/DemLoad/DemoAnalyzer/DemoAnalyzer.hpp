#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoAnalyzer final : public CSModuleBase {
    fs::path csdaPath;
    fs::path dirData;
    std::set<fs::path> analyzingSet;            // 正在分析的 demo 绝对路径

    void HandleAnalyzeRequest(fs::path demoPath);
    MulNX::CoTask AnalyzeDemoWithCSDA(fs::path demoPath);

    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};