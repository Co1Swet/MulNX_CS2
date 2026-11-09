#pragma once
#include <MulNX/MulNX.hpp>

class SysController final :public MulNX::Module<SysController> {
    bool Init()override;
    void Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& Msg)override;
    void Main();
};