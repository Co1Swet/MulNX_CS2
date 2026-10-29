#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class PassRecordMaker final :public DemModuleBase {
    MulNX::CoTask EnsureMake();
    
    bool Init()override;
};