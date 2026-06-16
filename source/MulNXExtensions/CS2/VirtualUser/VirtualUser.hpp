#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class VirtualUser final :public CSModuleBase {
public:
    std::atomic<bool> Enabled = true;
    bool Init()override;

    void Main();
    void ProcessMsg(MulNX::Message& Msg)override;
};