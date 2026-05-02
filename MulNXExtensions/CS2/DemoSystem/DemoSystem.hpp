#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoSystem final :public CSModuleBase {
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};