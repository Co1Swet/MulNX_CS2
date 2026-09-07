#pragma once
#include <Intro/CSModuleBase.hpp>

class ConsoleOutput final :public CSModuleBase {
    void ProcessMsg(MulNX::Message& msg);
    bool Init()override;    
};