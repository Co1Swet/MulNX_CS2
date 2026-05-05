#include "DemoRecorder.hpp"

#include <MulNXExtensions/CS2/CSController/CSController.hpp>

#include <string>
#include <utility>

// ========== 初始化 ==========
bool DemoRecorder::Init() {
    this->ISys()
        .SubscribeAsync("Demo/Record/Enqueue")
        .SubscribeAsync("Demo/Record/Reset")
        .SubscribeAsync("Demo/Record/Start")
        .SubscribeAsync("Demo/Record/Stop");

    this->SendTask("DemoRecorder", [this]()->bool {
        this->EntryProcessMsg();

        // 如果有协程正在运行，推它一把
        if (currentCoro.handle && !currentCoro.done()) {
            currentCoro.resume();
            return true;
        }
        else {
            // 启动新协程
            currentCoro = Main();
            // 因为 initial_suspend 是 suspend_always，需要第一次 resume
            if (!currentCoro.done()) {
                currentCoro.resume();
            }
        }

        return true;
        });

    return true;
}

// ========== 消息处理 ==========
void DemoRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Enqueue"_hash: {
        Steam64UID uid = msg.p1.as<Steam64UID>();
        int tick = msg.p2.low<int>();

        recordTaskBufferQueue.push_back({ uid, tick });
        break;
    }
    case "Demo/Record/Reset"_hash: {
        recordTaskBufferQueue.clear();

        // if (state != State::Idle) {
        //     StopRecording();
        //     state = State::Idle;
        //     currentWindow.reset();
        //     this->ISys().LogInfo("Reset: cleared queue and stopped current recording.");
        // }
        // else {
        //     this->ISys().LogInfo("Reset: queue cleared (no active recording).");
        // }
        break;
    }
    case "Demo/Record/Start"_hash: {
        moduleActive = true;
        this->ISys().LogInfo("Module activated.");
        break;
    }
    case "Demo/Record/Stop"_hash: {
        moduleActive = false;

        // if (state != State::Idle) {
        //     StopRecording();
        //     state = State::Idle;
        //     currentWindow.reset();
        //     this->ISys().LogInfo("Module deactivated, stopped current recording.");
        // }
        // else {
        //     this->ISys().LogInfo("Module deactivated.");
        // }
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
    return true;
}

Task DemoRecorder::Main() {
    // 1. 等待模块激活
    co_await AwaitCondition([this] { return this->moduleActive; });

    // 2. 等待队列中有任务
    RecordToDo task;
    co_await AwaitCondition([&] { return PeekQueue(task); });

    currentWindow = task;
    windowStartTick = task.tick - preRecordTicks;
    windowEndTick = task.tick + postRecordTicks;

    if (windowStartTick < 0) {
        this->ISys().LogWarning("Window start tick adjusted from "
            + std::to_string(windowStartTick) + " to 0.");
        windowStartTick = 0;
    }

    // 3. 发送跳转
    MulNX::Message gotoMsg("Demo/GotoTick"_hash);
    gotoMsg.p1.low<int>() = windowStartTick;
    this->ISys().PublishAsync(std::move(gotoMsg));

    // 4. 等待跳转完成
    co_await AwaitCondition([this] {
        return std::abs(this->CS2()->GetDemoTick() - windowStartTick) <= 10;
        });

    // 5. 固定等待 3 秒（后面换成异步）
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // 6. 设置观察目标
    MulNX::Message specMsg("Observe/SpecSteam64UID"_hash);
    specMsg.p1.as<Steam64UID>() = task.uid;
    this->ISys().PublishAsync(std::move(specMsg));

    this->ISys().LogInfo("Jumped to tick " + std::to_string(windowStartTick)
        + ", observing UID=" + std::to_string(task.uid));

    // 7. 开始录制
    MulNX::Message startMsg("Media/OBS/Record/Start"_hash);
    this->ISys().PublishAsync(std::move(startMsg));
    this->ISys().LogSucc("Recording started for UID="
        + std::to_string(currentWindow->uid)
        + " from tick " + std::to_string(windowStartTick)
        + " to " + std::to_string(windowEndTick));

    // 8. 等待录制结束 tick
    co_await AwaitCondition([this] {
        return this->CS2()->GetDemoTick() >= windowEndTick;
        });

    // 9. 停止录制
    StopRecording();
    this->ISys().LogSucc("Recording finished for UID="
        + std::to_string(currentWindow->uid));
    currentWindow.reset();
}

void DemoRecorder::StopRecording() {
    MulNX::Message stopMsg("Media/OBS/Record/Stop"_hash);
    this->ISys().PublishAsync(std::move(stopMsg));

    this->ISys().LogInfo("Sent OBS stop command.");
}