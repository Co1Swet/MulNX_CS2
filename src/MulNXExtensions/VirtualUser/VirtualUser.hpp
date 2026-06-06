#pragma once

#include <MulNX/MulNX.hpp>

class VirtualUser final :public MulNX::ModuleBase {
public:
    std::atomic<bool> Enabled = true;
    bool Init()override;

    void Main();
    void ProcessMsg(MulNX::Message& Msg)override;
};