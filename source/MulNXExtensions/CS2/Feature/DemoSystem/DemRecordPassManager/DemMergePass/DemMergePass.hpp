#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemMergePass final :public DemModuleBase {
    class RecordTaskConfiger* pConfiger = nullptr;
    bool Init()override;
public:
    void ApplyPass(std::vector<Demo::RecordTask>& rTasks)const;
};