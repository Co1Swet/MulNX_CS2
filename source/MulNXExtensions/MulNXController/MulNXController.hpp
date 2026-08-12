#pragma once
#include <MulNX/MulNX.hpp>

class MulNXController final :public MulNX::Module<MulNXController> {
    bool Init()override;
    void Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& Msg)override;
    void Main();
};