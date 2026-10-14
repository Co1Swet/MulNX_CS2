#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>
#include "DemMergePass/DemMergePass.hpp"

class DemRPassManager final :public DemModuleBase {
    std::vector<Demo::RecordTask>bufferTasks{};
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};