#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemoJSONReader final : public DemModuleBase {
    fs::path dirData;
    bool Window();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    void ReadJSON(const fs::path& filePath);
};