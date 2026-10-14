#include "DemRPassManager.hpp"

bool DemRPassManager::Init() {
    this->pDemClassifyPass = this->FindModule<DemClassifyPass>("DemClassifyPass");
    this->pDemSortPass = this->FindModule<DemSortPass>("DemSortPass");
    this->pDemMergePass = this->FindModule<DemMergePass>("DemMergePass");

    (*this)
        .SubscribeAsync("Demo/Record/Enqueue")
        .SubscribeAsync("Demo/Record/Commit")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemRPassManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Enqueue"_hash: {
        std::unique_lock lock(this->smutex);
        auto task = std::move(*msg.asp.get<Demo::RecordTask>());
        auto name = task.demoName;
        if (name.empty()) {
            this->LogError("不能入队空Demo名的录制任务！");
            break;
        }
        this->bufferTasks.push_back(std::move(task));
        break;
    }
    case "Demo/Record/Commit"_hash: {
        this->Commit();
        break;
    }
    }
}

void DemRPassManager::Commit() {
    auto rTasks = std::move(this->bufferTasks);
    if (rTasks.empty()) return;

    // 分类：形态从 vector<RecordTask> 转换为 vector<RecordPass>
    auto passes = this->pDemClassifyPass->ApplyPass(std::move(rTasks));
    for (auto& pass : passes) {
        this->pDemSortPass->ApplyPass(pass.rTasks);      // 就地对每个 demo 的任务排序
        this->pDemMergePass->ApplyPass(pass.rTasks);     // 依赖前一步的有序性
        // ...
    }
}