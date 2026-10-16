#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemWrapMaker final :public DemModuleBase {
    class RecordTaskConfiger* pConfiger = nullptr;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};