#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoJSONReader final : public CSModuleBase {
    fs::path dirData;
    bool Window();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void ReadJSON(const fs::path& filePath);
};