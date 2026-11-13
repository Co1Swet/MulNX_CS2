#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>
#include "DemClassifyPass/DemClassifyPass.hpp"
#include "DemMergePass/DemMergePass.hpp"
#include "DemSortPass/DemSortPass.hpp"

class DemRecordPassManager final :public DemModule<DemRecordPassManager> {
    DemClassifyPass* pDemClassifyPass = nullptr;
    DemSortPass* pDemSortPass = nullptr;
    DemMergePass* pDemMergePass = nullptr;
    std::vector<Demo::RecordTask>bufferTasks{};

    std::atomic<bool> setFocus = false;

    void Window();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
    void Commit(std::vector<Demo::RecordTask>&& rTasks, const int autoRecord)const;
};