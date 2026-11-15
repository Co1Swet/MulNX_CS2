#include "DemSortPass.hpp"

bool DemSortPass::Init() {
    
    return true;
}

void DemSortPass::ApplyPass(std::vector<Demo::Record::Task>& rTasks)const {
    const size_t count = rTasks.size();
    if (count < 2) return;

    std::stable_sort(rTasks.begin(), rTasks.end(),
        [](const Demo::Record::Task& a, const Demo::Record::Task& b) {
            if (a.tickStart != b.tickStart) return a.tickStart < b.tickStart;
            if (a.tickEnd != b.tickEnd)   return a.tickEnd < b.tickEnd;
            return a.uid < b.uid;
        });

    this->LogInfo(std::format(
        "排序完成：{} 条任务，tickStart {} ~ {}",
        count, rTasks.front().tickStart, rTasks.back().tickStart));
}