#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoInfoManager final : public CSModuleBase {
    std::filesystem::path currentDemoPath;
    std::filesystem::path csdaPath;
    std::filesystem::path outputDir;
    void AnalyzeDemoWithCSDA();
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};