#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoAnalyzer final : public CSModuleBase {
    std::filesystem::path csdaPath{};
    std::filesystem::path dirData{};
    MulNX::CoTask AnalyzeDemoWithCSDA(std::filesystem::path&& pthDemo);
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};