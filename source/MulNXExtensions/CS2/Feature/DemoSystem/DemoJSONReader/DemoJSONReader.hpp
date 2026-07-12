#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoJSONReader final : public CSModuleBase {
    std::filesystem::path dirData;
    bool Window();
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};