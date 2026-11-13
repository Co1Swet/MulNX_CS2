#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemoJSONReader final : public DemModule<DemoJSONReader> {
    fs::path dirData;
    bool Window();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
    
    void OnJSONMsg(MulNX::Message& msg);
    void ReadJSON(std::string&& kDir, const fs::path& filePath);
};