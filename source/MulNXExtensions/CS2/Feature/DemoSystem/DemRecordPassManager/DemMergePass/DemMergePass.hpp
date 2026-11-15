#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

class DemMergePass final :public DemModule<DemMergePass> {
    class RecordTaskConfiger* pConfiger = nullptr;
    bool Init()override;
public:
    void ApplyPass(std::vector<Demo::Record::Task>& rTasks)const;
};