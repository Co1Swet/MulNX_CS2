#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class RecordTaskConfiger;
class RecordTaskMaker final :public DemModuleBase {
    RecordTaskConfiger* pConfiger = nullptr;
    std::map<std::string, Demo::Info>demos;

    bool showBekillEvent = true;
    bool DemoChooseMenu(Demo::Info& demoInfo);
    void Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;
};