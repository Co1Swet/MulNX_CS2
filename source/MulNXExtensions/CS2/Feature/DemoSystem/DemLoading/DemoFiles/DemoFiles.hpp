#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemoFiles :public DemModuleBase {
    struct DemoFile {
        std::string kDir{};
        std::string demoName{};
        bool analyzed = false;
        bool beChoosing = false;
    };
    std::vector<DemoFile> demoFiles{};
    fs::path dirData{};

    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};