#include "DemClassifyPass.hpp"
#include <unordered_map>

bool DemClassifyPass::Init() {

    return true;
}

std::vector<Demo::RecordPass> DemClassifyPass::ApplyPass(
    std::vector<Demo::RecordTask>&& rTasks)const {
    const size_t inputCount = rTasks.size();
    this->LogInfo(std::format("开始分类：输入 {} 条录制任务", inputCount));

    std::vector<Demo::RecordPass> passes;
    if (rTasks.empty()) {
        this->LogInfo("分类跳过：输入任务为空");
        return passes;
    }

    // 记录每个 demoName 在 passes 中的位置，保证 demo 顺序 = 首次出现顺序
    std::unordered_map<std::string, size_t> indexByDemo;
    indexByDemo.reserve(inputCount);

    size_t droppedCount = 0;
    for (auto& task : rTasks) {
        if (task.demoName.empty()) {
            this->LogWarning(std::format(
                "丢弃无 demoName 的任务：uid={}, tick {}~{}",
                task.uid, task.tickStart, task.tickEnd));
            ++droppedCount;
            continue;
        }

        auto [it, inserted] = indexByDemo.emplace(task.demoName, passes.size());
        if (inserted) {
            Demo::RecordPass pass;
            pass.demoFileName = task.demoName;
            pass.rTasks.reserve(1);
            pass.rTasks.push_back(std::move(task));
            passes.push_back(std::move(pass));
        }
        else {
            passes[it->second].rTasks.push_back(std::move(task));
        }
    }

    this->LogInfo(std::format(
        "分类完成：{} 条任务 -> {} 个 RecordPass（丢弃 {} 条）",
        inputCount, passes.size(), droppedCount));

    for (const auto& pass : passes) {
        this->LogInfo(std::format("  [{}] {} 条任务",
            pass.demoFileName, pass.rTasks.size()));
    }

    return passes;
}