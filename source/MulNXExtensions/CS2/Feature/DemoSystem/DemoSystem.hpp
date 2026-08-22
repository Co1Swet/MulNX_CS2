#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoSystem final :public CSModuleBase {
    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};