#pragma once
#include <Intro/Intros.hpp>

class VirtualUser final :public CSModule<VirtualUser> {
    MulReflBool(enabled, true);

    bool Init()override;
    void Main();
    void ProcessMsg(MulNX::Message& Msg)override;
};