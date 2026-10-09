#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class DemoFiles :public DemModuleBase {
    struct DemoFile {
        fs::path path{};
        bool anylized = false;
        bool beChoosing = false;
    };
    std::vector<DemoFile> demoFiles{};
    fs::path dirData{};

    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};