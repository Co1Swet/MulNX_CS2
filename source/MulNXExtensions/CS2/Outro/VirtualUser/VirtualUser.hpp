#pragma once
#include <Intro/Intros.hpp>

class VirtualUser final :public CSModuleBase {
    std::atomic<bool> Enabled = true;
    bool Init()override;

    void Main();
    void ProcessMsg(MulNX::Message& Msg)override;
};