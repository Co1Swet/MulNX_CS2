#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class DemEasyView final :public DemModuleBase {
    std::atomic<Steam64UID> localUid = 0;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};