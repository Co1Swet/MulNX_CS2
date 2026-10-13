#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class DemOneKillMaker final :public DemModuleBase {
    class RecordTaskConfiger* pConfiger = nullptr;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};