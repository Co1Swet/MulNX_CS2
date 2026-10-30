#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class DemSortPass final : public DemModuleBase {
    bool Init() override;

public:
    // 就地排序：按 tickStart 升序，让录制器的播放头尽量单调前进
    void ApplyPass(std::vector<Demo::RecordTask>& rTasks)const;
};