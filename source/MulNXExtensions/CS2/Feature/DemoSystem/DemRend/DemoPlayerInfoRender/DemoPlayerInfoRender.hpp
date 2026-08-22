#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class DemoPlayerInfoRender final :public CSModuleBase {
    std::map<Steam64UID, std::string>crosshairShareCodes;

    void Menu(MulNX::Message* umsg);
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;
};