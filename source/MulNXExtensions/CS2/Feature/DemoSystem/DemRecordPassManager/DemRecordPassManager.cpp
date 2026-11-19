#include "DemRecordPassManager.hpp"
#include <MulNXUtils/Times/Times.hpp>

void DemRecordPassManager::Window() {
    auto w = MulNX::UI::RAIIWindow("预备录制队列");
    if (this->setFocus.load(std::memory_order_acquire)) {
        ImGui::SetWindowFocus("预备录制队列");
        this->setFocus.store(false, std::memory_order_release);
    }
    if (!w || !w.ShouldDraw())return;

    MulNX::UI::Checkbox("录制完成后自动拼接", this->autoConcat);
    ImGui::Separator();

    if (ImGui::Button("清空队列")) {
        this->PublishAsync("Demo/Record/Clear"_hash);
    }

    if (ImGui::Button("提交所有任务")) {
        MulNX::Message msg("Demo/Record/Commit/Demo"_hash);
        auto&& [autoRecord, forceConcat] = msg.Access<bool, bool>();
        autoRecord = false;forceConcat = false;
        this->PublishAsync(std::move(msg));
        ImGui::SetWindowFocus("录制执行");
    }
    ImGui::SameLine();
    if (ImGui::Button("提交所有任务并开始录制")) {
        MulNX::Message msg("Demo/Record/Commit/Demo"_hash);
        auto&& [autoRecord, forceConcat] = msg.Access<bool, bool>();
        autoRecord = true;forceConcat = false;
        this->PublishAsync(std::move(msg));
        ImGui::SetWindowFocus("录制执行");
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

bool DemRecordPassManager::Init() {
    this->pDemClassifyPass = this->FindModule<DemClassifyPass>();
    this->pDemSortPass = this->FindModule<DemSortPass>();
    this->pDemMergePass = this->FindModule<DemMergePass>();

    (*this)
        .SubscribeAsync("Demo/Record/Enqueue")
        .SubscribeAsync<size_t>("Demo/Record/Remove")
        .SubscribeAsync("Demo/Record/Clear")
        .SubscribeAsync("Demo/Record/Commit")
        ;

    this->SubscribeSync("Demo/Record/Task/End", [this](MulNX::Message& msg) {
        this->OnTaskEnd(msg);
        });

    this->UIRegisterCallback("UI.Demos", [this](auto&&...) {
        this->Window();
        });

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemRecordPassManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Enqueue"_hash: {
        std::unique_lock lock(this->smutex);
        this->setFocus.store(true, std::memory_order_release);
        auto task = std::move(*msg.asp.get<Demo::Record::Task>());
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
        auto&& [autoRecord, forceConcat] = msg.Access<bool, bool>();
        std::vector<Demo::Record::Task> rTasks;
        {
            std::unique_lock lock(this->smutex);
            rTasks = std::move(this->bufferTasks);
            this->bufferTasks = {};
        }
        if (rTasks.empty())break;
        auto token = std::make_shared<Demo::Record::Token>([&](Demo::Record::Token* token) {
            this->OnTokenDestroy(token);
            });
        this->Commit(std::move(rTasks), autoRecord,
            token, this->autoConcat.load() || forceConcat);
        break;
    }
    }
}

void DemRecordPassManager::Commit(std::vector<Demo::Record::Task>&& rTasks,
    const bool autoRecord, std::shared_ptr<Demo::Record::Token> token, const bool autoConcat)const {

    token->createdAt = std::chrono::system_clock::now();
    token->createdAtStr = MulNX::Time::ToTimestamp(token->createdAt);
    if (autoConcat) {
        token->concatTarget = this->Path()->PathGetForShared("Videos") / (token->createdAtStr + ".mp4");
    }
    
    std::vector<MulNX::Message> buffer{};
    // 分类：形态从 vector<Task> 转换为 vector<Pass>
    auto passes = this->pDemClassifyPass->ApplyPass(std::move(rTasks));
    for (auto& pass : passes) {
        this->pDemSortPass->ApplyPass(pass.rTasks); // 就地对每个 demo 的任务排序
        this->pDemMergePass->ApplyPass(pass.rTasks); // 依赖前一步的有序性
        this->pDemSortPass->ApplyPass(pass.rTasks); // 再跑一次排序

        auto [msg, rp] = MulNX::Message::Create<Demo::Record::Pass>("Demo/Pass/Commit"_hash);
        pass.token = token;
        *rp = std::move(pass);
        buffer.push_back(std::move(msg));
    }

    for (auto& msg : buffer) {
        ++token->passCount;
        this->PublishAsync(std::move(msg));
    }
   
    this->LogInfo(std::format("已提交 {} 个 Record::Pass 到录制器", passes.size()));
    if (!autoRecord)return;
    this->PublishAsync("Demo/Pass/Start"_hash);
    this->LogInfo("根据设置自动触发录制");
}
void DemRecordPassManager::OnTaskEnd(MulNX::Message& msg) {
    auto&& [recordSucc, pPass, iIndex, drop, pOutputDir] =
        msg.Access<bool, const Demo::Record::Pass*, size_t, bool, const fs::path*>();
    
    auto token = pPass->token;
    const auto& task = pPass->rTasks.at(iIndex);

    if (!recordSucc) {
        this->LogError(std::format("{} 录制失败，不参与拼接", task.desc));
        return;
    }
    if (!token->concatTarget.has_value())return;
    
    auto [add, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/Add"_hash);
    rp->str1 = token->concatTarget.value();
    rp->str2 = (*pOutputDir / (task.fileNameNoExt + ".mp4")).string();
    this->PublishAsync(std::move(add));

    return;
}
void DemRecordPassManager::OnTokenDestroy(Demo::Record::Token* token) {
    this->LogInfo(std::format("令牌正在析构！ID为：{}", token->createdAtStr));
    if (token->passCount == 0)return;
    if (!token->concatTarget.has_value())return;

    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/End"_hash);
    rp->str1 = token->concatTarget.value();
    this->PublishAsync(std::move(msg));
}