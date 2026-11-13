#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemEasyView final :public DemModule<DemEasyView> {
    std::atomic<Steam64UID> localUid = 0;
    std::string localName{};
    void Window();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};