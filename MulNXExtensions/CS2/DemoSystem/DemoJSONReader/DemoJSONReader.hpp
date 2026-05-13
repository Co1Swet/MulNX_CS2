#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoJSONReader final : public CSModuleBase {
    std::filesystem::path dirDemos;
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};