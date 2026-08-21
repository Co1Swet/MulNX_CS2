#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoAnalyzer final : public CSModuleBase {
    std::filesystem::path csdaPath;
    std::filesystem::path dirData;
    std::set<std::filesystem::path> analyzingSet;            // 正在分析的 demo 绝对路径

    void HandleAnalyzeRequest(std::filesystem::path demoPath);
    MulNX::CoTask AnalyzeDemoWithCSDA(std::filesystem::path demoPath);

    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};