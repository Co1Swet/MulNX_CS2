#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ConsoleOutput final :public CSModuleBase {
    void ProcessMsg(MulNX::Message& msg);
    bool Init()override;    
};