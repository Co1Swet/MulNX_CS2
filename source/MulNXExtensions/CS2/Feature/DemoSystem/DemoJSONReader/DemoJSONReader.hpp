#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoJSONReader final : public CSModuleBase {
    std::filesystem::path dirData;
    bool Window();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void ReadJSON(const std::filesystem::path& filePath);
};