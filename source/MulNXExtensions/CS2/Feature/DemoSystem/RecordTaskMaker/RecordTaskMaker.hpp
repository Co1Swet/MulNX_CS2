#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Feature/DemoSystem/DemoStruct.hpp>

class RecordTaskConfiger;
class RecordTaskMaker final :public CSModuleBase {
    RecordTaskConfiger* pConfiger = nullptr;
    std::map<std::string, Demo::Info>demos;
    std::string currentDemoName;

    bool showBekillEvent = true;
    bool Window(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& msg)override;
public:
    bool Init()override;
};