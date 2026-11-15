#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

class DemSortPass final : public DemModule<DemSortPass> {
    bool Init() override;

public:
    // 就地排序：按 tickStart 升序，让录制器的播放头尽量单调前进
    void ApplyPass(std::vector<Demo::RecordTask>& rTasks)const;
};