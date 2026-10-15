#include "DemRPassManager.hpp"

void DemRPassManager::Window() {
    auto w = MulNX::UI::RAIIWindow("预备录制队列");
    if (!w || !w.ShouldDraw())return;

    if (ImGui::Button("清空队列")) {
        this->PublishAsync("Demo/Record/Clear"_hash);
    }

    if (ImGui::Button("提交所有任务")) {
        MulNX::Message msg("Demo/Record/Make/FlushHigh"_hash);
        auto&& [autoRecord] = msg.Access<int>();
        autoRecord = 0;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("提交所有任务并开始录制")) {
        MulNX::Message msg("Demo/Record/Make/FlushHigh"_hash);
        auto&& [autoRecord] = msg.Access<int>();
        autoRecord = 1;
        this->PublishAsync(std::move(msg));
    }


    ImGui::SeparatorText("缓冲队列");
    auto btn = MulNX::UI::SmartButton(0);
    std::shared_lock lock(this->smutex);
    for (size_t i = 0;i < this->bufferTasks.size();++i) {
        const auto& task = this->bufferTasks[i];
        ImGui::Text("%s", task.desc.c_str());
        ImGui::SameLine();
        if (btn.Next("移除")) {
            MulNX::Message msg("Demo/Record/Remove"_hash);
            auto&& [index] = msg.Access<size_t>();
            index = i;
            this->PublishAsync(std::move(msg));
        }
    }
}

bool DemRPassManager::Init() {
    this->pDemClassifyPass = this->FindModule<DemClassifyPass>("DemClassifyPass");
    this->pDemSortPass = this->FindModule<DemSortPass>("DemSortPass");
    this->pDemMergePass = this->FindModule<DemMergePass>("DemMergePass");

    (*this)
        .SubscribeAsync("Demo/Record/Enqueue")
        .SubscribeAsync<size_t>("Demo/Record/Remove")
        .SubscribeAsync("Demo/Record/Clear")
        .SubscribeAsync("Demo/Record/Commit")
        ;

    this->UIRegisterCallback("UI.Demos", [this](auto&&...) {
        this->Window();
        });

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
    case "Demo/Record/Remove"_hash: {
        std::unique_lock lock(this->smutex);
        auto&& [index] = msg.Access<size_t>();
        if (index >= this->bufferTasks.size()) {
            this->LogError(std::format("请求移除缓冲区下标越界：index={}, size={}", index, this->bufferTasks.size()));
            break;
        }
        this->LogInfo(std::format("移除缓冲任务：{}", this->bufferTasks[index].desc));
        this->bufferTasks.erase(this->bufferTasks.begin() + index);
        break;
    }
    case "Demo/Record/Clear"_hash: {
        std::unique_lock lock(this->smutex);
        const size_t n = this->bufferTasks.size();
        this->bufferTasks.clear();
        this->LogInfo(std::format("清空缓冲队列，丢弃 {} 条任务", n));
        break;
    }
    case "Demo/Record/Commit"_hash: {
        std::unique_lock lock(this->smutex);
        auto&& [autoRecord] = msg.Access<int>();
        this->Commit(autoRecord);
        break;
    }
    }
}

void DemRPassManager::Commit(const int autoRecord) {
    auto rTasks = std::move(this->bufferTasks);
    if (rTasks.empty()) return;

    // 分类：形态从 vector<RecordTask> 转换为 vector<RecordPass>
    auto passes = this->pDemClassifyPass->ApplyPass(std::move(rTasks));
    for (auto& pass : passes) {
        this->pDemSortPass->ApplyPass(pass.rTasks); // 就地对每个 demo 的任务排序
        this->pDemMergePass->ApplyPass(pass.rTasks); // 依赖前一步的有序性
        this->pDemSortPass->ApplyPass(pass.rTasks); // 再跑一次排序

        auto [msg, rp] = MulNX::Message::Create<Demo::RecordPass>("Demo/Pass/Commit"_hash);
        *rp = std::move(pass);
        this->PublishAsync(std::move(msg));
    }

    this->LogInfo(std::format("已提交 {} 个 RecordPass 到录制器", passes.size()));
    if (autoRecord <= 0)return;
    this->PublishAsync("Demo/Pass/Start"_hash);
    this->LogInfo("根据设置自动触发录制");
}