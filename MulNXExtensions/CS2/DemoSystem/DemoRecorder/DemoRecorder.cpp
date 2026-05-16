#include "DemoRecorder.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/TimeController/TimeController.hpp>

bool DemoRecorder::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo Recorder", this->showWindow);
    if (!w) return true;

    if (ImGui::Button("启动")) {
        this->ISys().PublishAsync("Demo/Record/Start"_hash);
    }

    return true;
}

// ========== 初始化 ==========
bool DemoRecorder::Init() {
    this->ISys()
        .SubscribeAsync("Demo/Record/Enqueue")
        .SubscribeAsync("Demo/Record/Reset")
        .SubscribeAsync("Demo/Record/Start")
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

// ========== 消息处理 ==========
void DemoRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Enqueue"_hash: {
        Steam64UID uid = msg.p1.as<Steam64UID>();
        int tick = msg.p2.low<int>();
        this->recordTaskBufferQueue.push_back({ uid, tick });
        this->isEmpty.store(false, std::memory_order_release);
        break;
    }
    case "Demo/Record/Reset"_hash: {
        this->recordTaskBufferQueue.clear();
        this->isEmpty.store(true, std::memory_order_release);
        break;
    }
    case "Demo/Record/Start"_hash: {
        this->moduleActive = true;
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

// ========== 队列操作（无锁，调用者持有 mtx） ==========
bool DemoRecorder::PeekQueue(RecordToDo& task) {
    if (recordTaskBufferQueue.empty()) return false;
    task = recordTaskBufferQueue.front();
    recordTaskBufferQueue.pop_front();
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
        RecordToDo task;
        co_await this->WaitUntil([&]()->bool { return PeekQueue(task); });

        currentWindow = task;
        windowStartTick = task.tick - preRecordTicks;
        windowEndTick = task.tick + postRecordTicks;

        if (windowStartTick < 0) {
            this->ISys().LogWarning("Window start tick adjusted from "
                + std::to_string(windowStartTick) + " to 0.");
            windowStartTick = 0;
        }

        // 暂停并跳转
        this->ISys().AsyncCommand("demo_pause");
        MulNX::Message gotoMsg("Demo/GotoTick"_hash);
        gotoMsg.p1.low<int>() = windowStartTick;
        this->ISys().PublishAsync(std::move(gotoMsg));

        // 等待跳转完成
        co_await this->WaitUntil([this] {
            return std::abs(this->CS2Time()->GetDemoTick() - windowStartTick) <= 5;
            });

        // 等待加载
        auto current = this->CS2Time()->GetDemoTick();
        this->ISys().AsyncCommand("demo_resume");
        co_await this->WaitUntil([&] {
            return this->CS2Time()->GetDemoTick() > current + 10;
            });

        // 设置观察目标
        MulNX::Message specMsg("Observe/SpecSteam64UID"_hash);
        specMsg.p1.as<Steam64UID>() = task.uid;
        this->ISys().PublishAsync(std::move(specMsg));

        this->ISys().LogInfo("Jumped to tick " + std::to_string(windowStartTick)
            + ", observing UID=" + std::to_string(task.uid));

        // 开始录制
        if (this->newStart.load(std::memory_order_acquire)) {
            this->ISys().PublishAsync("Media/Record/Start"_hash);
            this->newStart.store(false, std::memory_order_release);
        }
        else {
            this->ISys().PublishAsync("Media/Record/Resume"_hash);
        }
        this->ISys().LogSucc("Recording started for UID="
            + std::to_string(currentWindow->uid)
            + " from tick " + std::to_string(windowStartTick)
            + " to " + std::to_string(windowEndTick));

        // 等待录制结束 tick
        co_await this->WaitUntil([this] {
            return this->CS2Time()->GetDemoTick() >= windowEndTick;
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
            + std::to_string(currentWindow->uid));
        currentWindow.reset();
    }
    co_return;
}