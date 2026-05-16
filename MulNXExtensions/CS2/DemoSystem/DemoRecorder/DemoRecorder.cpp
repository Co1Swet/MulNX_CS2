#include "DemoRecorder.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/TimeController/TimeController.hpp>
#include <MulNXExtensions/CS2/DemoSystem/DemoJSONReader/DemoJSONReader.hpp>

bool DemoRecorder::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo Recorder", this->showWindow);
    if (!w) return true;

    if (ImGui::Button("启动")) {
        this->ISys().PublishAsync("Demo/Record/Start"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("清空队列")) {
        this->ISys().PublishAsync("Demo/Record/Clear"_hash);
    }

    ImGui::Text("当前队列：");
    std::shared_lock lock(this->smutex);
    for (const auto& recordTask : this->recordTaskBufferQueue) {
        ImGui::Text(recordTask->GetDesc().c_str());
    }

    return true;
}

bool DemoRecorder::Init() {
    this->pJSON = this->Core->ModuleManager()->FindModule<DemoJSONReader>("DemoJSONReader");

    this->ISys()
        .SubscribeAsync("Demo/Record/Enqueue")
        .SubscribeAsync("Demo/Record/Start")
        .SubscribeAsync("Demo/Record/Clear")
        .SubscribeAsync("Demo/Record/Stop");

    this->coTa = Main();
    this->coTa.resume();

    this->SendTask("DemoSys", [this]()->bool {
        this->Update();
        return true;
        });

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        return this->Window(node);
        });
    this->showWindow.store(true, std::memory_order_release);

    return true;
}

void DemoRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Enqueue"_hash: {
        std::unique_lock lock(this->smutex);
        auto task = std::move(*msg.asp.reinterpret_cast_get<std::unique_ptr<IRecordTask>>());
        this->recordTaskBufferQueue.push_back(std::move(task));
        this->isEmpty.store(false, std::memory_order_release);
        break;
    }
    case "Demo/Record/Clear"_hash: {
        std::unique_lock lock(this->smutex);
        this->recordTaskBufferQueue.clear();
        this->isEmpty.store(true, std::memory_order_release);
        break;
    }
    case "Demo/Record/Start"_hash: {
        this->moduleActive.store(true, std::memory_order_release);
        this->newStart.store(true, std::memory_order_release);
        this->ISys().LogInfo("Module activated.");
        break;
    }
    case "Demo/Record/Stop"_hash: {
        this->moduleActive.store(false, std::memory_order_release);
        break;
    }
    default:
        break;
    }
}

bool DemoRecorder::PeekQueue(std::unique_ptr<IRecordTask>& task) {
    std::unique_lock lock(this->smutex);
    if (this->recordTaskBufferQueue.empty()) return false;
    task = std::move(this->recordTaskBufferQueue.front());
    this->recordTaskBufferQueue.pop_front();
    if (this->recordTaskBufferQueue.empty()) {
        this->isEmpty.store(true, std::memory_order_release);
        this->moduleActive.store(false, std::memory_order_release);
    }
    else {
        this->isEmpty.store(false, std::memory_order_release);
    }
    return true;
}

MulNX::CoTask DemoRecorder::Main() {
    while (true) {
        // 等待模块激活
        co_await this->WaitUntil([this]()->bool { return this->moduleActive.load(std::memory_order_acquire); });

        // 等待队列中有任务
        std::unique_ptr<IRecordTask> task = nullptr;
        co_await this->WaitUntil([&]()->bool { return this->PeekQueue(task); });

        this->currentRecordTaskStartTick = task->GetTargetTick() - this->preRecordTicks;
        this->currentRecordTaskEndTick = task->GetTargetTick() + postRecordTicks;
        auto uid = task->GetTargetSteam64UID();
        this->currentRecordTask = std::move(task);


        if (this->currentRecordTaskStartTick < 0) {
            this->ISys().LogWarning("Window start tick adjusted from "
                + std::to_string(this->currentRecordTaskStartTick) + " to 0.");
            this->currentRecordTaskStartTick = 0;
        }

        // 暂停并跳转
        this->ISys().AsyncCommand("demo_pause");
        MulNX::Message gotoMsg("Demo/GotoTick"_hash);
        gotoMsg.p1.low<int>() = this->currentRecordTaskStartTick;
        this->ISys().PublishAsync(std::move(gotoMsg));

        // 等待跳转完成
        co_await this->WaitUntil([this] {
            return std::abs(this->CS2Time()->GetDemoTick() - this->currentRecordTaskStartTick) <= 5;
            });

        // 等待加载
        auto current = this->CS2Time()->GetDemoTick();
        this->ISys().AsyncCommand("demo_resume");
        co_await this->WaitUntil([&] {
            return this->CS2Time()->GetDemoTick() > current + 10;
            });

        // 设置观察目标
        MulNX::Message specMsg("Observe/SpecSteam64UID"_hash);
        specMsg.p1.as<Steam64UID>() = uid;
        this->ISys().PublishAsync(std::move(specMsg));

        this->ISys().LogInfo("Jumped to tick " + std::to_string(this->currentRecordTaskStartTick)
            + ", observing UID=" + std::to_string(uid));

        // 开始录制
        if (this->newStart.load(std::memory_order_acquire)) {
            this->ISys().PublishAsync("Media/Record/Start"_hash);
            this->newStart.store(false, std::memory_order_release);
        }
        else {
            this->ISys().PublishAsync("Media/Record/Resume"_hash);
        }
        this->ISys().LogSucc("Recording started for UID="
            + std::to_string(uid)
            + " from tick " + std::to_string(this->currentRecordTaskStartTick)
            + " to " + std::to_string(this->currentRecordTaskEndTick));

        // 等待录制结束 tick
        co_await this->WaitUntil([this] {
            return this->CS2Time()->GetDemoTick() >= this->currentRecordTaskEndTick;
            });

        // 暂停或停止录制
        if (this->isEmpty.load(std::memory_order_acquire)) {
            this->ISys().PublishAsync("Media/Record/Stop"_hash);
        }
        else {
            this->ISys().PublishAsync("Media/Record/Pause"_hash);
        }

        this->ISys().LogInfo("Sent OBS stop command.");
        this->ISys().LogSucc("Recording finished for UID="
            + std::to_string(uid));
        this->currentRecordTask.reset();
    }
    co_return;
}