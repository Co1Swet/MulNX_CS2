#pragma once
#include <Intro/CSModuleBase.hpp>

class DemState final :public CSModuleBase {
    
    void Menu()const;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
public:
    
};