#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Feature/DemoSystem/DemoStruct.hpp>

class DemoPlayerInfoRender final :public CSModuleBase {
    std::map<Steam64UID, std::string>crosshairShareCodes;

    void Menu(MulNX::Message* umsg);
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;
};