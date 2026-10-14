#include "DemMergePass.hpp"
#include <Feature/DemoSystem/DemRecord/RecordTaskConfiger/RecordTaskConfiger.hpp>

bool DemMergePass::Init() {
    this->pConfiger = this->FindModule<RecordTaskConfiger>("RecordTaskConfiger");
    return true;
}

void DemMergePass::ApplyPass(std::vector<Demo::RecordTask>& rTasks) {
    this->LogInfo("开始执行合并优化");
    if (rTasks.size() < 2) return;

    const int pre = this->pConfiger->preRecordTicks;
    const int post = this->pConfiger->postRecordTicks;
    const int gap = std::max(0, this->pConfiger->GetMergeThresholdTicks() - (pre + post));

    auto sameDomain = [&](const Demo::RecordTask& a, const Demo::RecordTask& b) {
        return a.demoName == b.demoName
            && a.uid == b.uid
            && a.roundNumber == b.roundNumber
            && (b.tickStart - a.tickEnd) <= gap;
        };

    auto mergeWindow = [](Demo::RecordTask& dst, const Demo::RecordTask& src) {
        dst.tickStart = std::min(dst.tickStart, src.tickStart);
        dst.tickEnd = std::max(dst.tickEnd, src.tickEnd);
        };

    std::vector<Demo::RecordTask> out;
    std::vector<int>              counts;
    out.reserve(rTasks.size());
    counts.reserve(rTasks.size());

    for (auto& t : rTasks) {
        if (!out.empty() && sameDomain(out.back(), t)) {
            mergeWindow(out.back(), t);
            ++counts.back();
        }
        else {
            out.push_back(std::move(t));
            counts.push_back(1);
        }
    }

    // 统一重写描述：带上杀手 uid / 名字
    for (size_t i = 0; i < out.size(); ++i) {
        if (counts[i] <= 1) continue;

        auto& task = out[i];
        std::string killer = std::to_string(task.uid);
        if (auto info = this->pDemState->GetDemoInfo(task.demoName)) {
            killer = info->GetPlayerName(task.uid);
        }

        task.desc = std::format("连续 {} 击杀（{}，uid {}），tick {} ~ {}",
            counts[i], killer, task.uid, task.tickStart, task.tickEnd);
    }

    this->LogInfo(std::format("合并完成：{} -> {}", rTasks.size(), out.size()));
    rTasks = std::move(out);
}