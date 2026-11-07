#pragma once
#include <Intro/Intros.hpp>

class ConsoleOutput final :public CSModuleBase {
    std::atomic<bool> enable = false;
    void ProcessMsg(MulNX::Message& msg);
    bool Init()override;    
};