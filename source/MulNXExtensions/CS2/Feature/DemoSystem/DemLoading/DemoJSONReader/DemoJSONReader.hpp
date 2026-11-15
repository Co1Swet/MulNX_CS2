#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

class DemoJSONReader final : public DemModule<DemoJSONReader> {
    fs::path dirData;
    bool Window();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    
    void OnJSONMsg(MulNX::Message& msg);
    void ReadJSON(const std::string& kDir, const fs::path& filePath);
};