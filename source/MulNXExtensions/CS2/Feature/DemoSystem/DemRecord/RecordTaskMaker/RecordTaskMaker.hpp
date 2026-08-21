#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Feature/DemoSystem/DemoStruct.hpp>

class RecordTaskConfiger;
class RecordTaskMaker final :public CSModuleBase {
    RecordTaskConfiger* pConfiger = nullptr;
    std::map<std::string, Demo::Info>demos;
    std::string currentDemoName;

    bool showBekillEvent = true;
    void Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;
};