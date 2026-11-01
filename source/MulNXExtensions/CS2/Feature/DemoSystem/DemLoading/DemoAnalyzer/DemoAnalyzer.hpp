#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemoAnalyzer final : public DemModuleBase {
    fs::path csdaPath;
    fs::path dirData;
    std::set<fs::path> analyzingSet;            // 正在分析的 demo 绝对路径

    MulNX::CoTask OnAnalyzeRequest(MulNX::Message& msg);
    MulNX::CoTask HandleAnalyzeRequest(const std::string& key, const std::string& demoName, const fs::path& demoPath);
    MulNX::CoTask AnalyzeDemoWithCSDA(const std::string& key, const std::string& demoName, const fs::path& demoPath);

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};