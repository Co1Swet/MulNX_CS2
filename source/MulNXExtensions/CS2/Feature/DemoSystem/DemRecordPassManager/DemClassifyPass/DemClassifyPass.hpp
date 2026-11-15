#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

class DemClassifyPass final : public DemModule<DemClassifyPass> {
    bool Init() override;
public:
    // 接收右值任务列表，按 demoName 分类为多个 RecordPass
    // 每个 RecordPass 内保留任务的原始相对顺序
    std::vector<Demo::Record::Pass> ApplyPass(std::vector<Demo::Record::Task>&& rTasks)const;
};