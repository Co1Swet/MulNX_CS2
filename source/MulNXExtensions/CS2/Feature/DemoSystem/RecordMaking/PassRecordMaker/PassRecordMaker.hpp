#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class PassRecordMaker final :public DemModuleBase {
    MulNX::CoTask AsyncWaitInfo(bool& succ, const std::string& demoName);
    MulNX::CoTask EnsureMake(std::string&& demoName);

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};