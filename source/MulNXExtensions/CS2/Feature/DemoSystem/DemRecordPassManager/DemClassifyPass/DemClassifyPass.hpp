#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemClassifyPass final : public DemModuleBase {
    bool Init() override;
public:
    // 接收右值任务列表，按 demoName 分类为多个 RecordPass
    // 每个 RecordPass 内保留任务的原始相对顺序
    std::vector<Demo::RecordPass> ApplyPass(std::vector<Demo::RecordTask>&& rTasks)const;
};