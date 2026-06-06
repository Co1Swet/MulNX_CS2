#pragma once

#include <MulNX/MulNX.hpp>

class MulNXController final :public MulNX::ModuleBase {
    bool Init()override;
    bool UINodeFunc(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& Msg)override;
    void Main();
};