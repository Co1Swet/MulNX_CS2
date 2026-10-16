#include "DemMergePass.hpp"
#include <Feature/DemoSystem/RecordTaskConfiger/RecordTaskConfiger.hpp>

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
    std::vector<int>              lastKillIdx;      // 平行数组：合并块内最后一个 killIndex
    out.reserve(rTasks.size());
    counts.reserve(rTasks.size());
    lastKillIdx.reserve(rTasks.size());

    for (auto& t : rTasks) {
        if (!out.empty() && sameDomain(out.back(), t)) {
            mergeWindow(out.back(), t);
            ++counts.back();
            lastKillIdx.back() = t.killIndex;       // 已排序，后者一定更大
        }
        else {
            out.push_back(std::move(t));
            counts.push_back(1);
            lastKillIdx.push_back(out.back().killIndex);   // move 后再取，安全
        }
    }

    for (size_t i = 0; i < out.size(); ++i) {
        if (counts[i] <= 1) continue;

        auto& task = out[i];

        std::string killer = std::to_string(task.uid);
        if (auto info = this->pDemSys->GetDemoInfo(task.demoName)) {
            killer = info->GetPlayerName(task.uid);
        }

        const int firstIdx = task.killIndex;    // push 时写入，merge 时未动，天然是"首"
        const int lastIdx = lastKillIdx[i];    // 平行数组给出"尾"

        task.desc = std::format(
            "第 {} 回合连续 {} 击杀（{}，uid {}），击杀序号 {}-{} / 共 {}，tick {} ~ {}",
            task.roundNumber, counts[i], killer, task.uid,
            firstIdx, lastIdx, task.roundKill,
            task.tickStart, task.tickEnd);

        // 1 和 3 → "13"（首尾直接拼接）
        task.fileNameNoExt = std::format("{}_{}_r{}_kill_{}{}_{}",
            task.demoName, task.uid, task.roundNumber,
            firstIdx, lastIdx, task.roundKill);
    }

    this->LogInfo(std::format("合并完成：{} -> {}", rTasks.size(), out.size()));
    rTasks = std::move(out);
}