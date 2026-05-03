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
        this->Main();
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

        std::lock_guard<std::mutex> lock(mtx);
        recordTaskBufferQueue.push_back({ uid, tick });
        break;
    }
    case "Demo/Record/Reset"_hash: {
        std::lock_guard<std::mutex> lock(mtx);
        recordTaskBufferQueue.clear();

        if (state != State::Idle) {
            StopRecording();
            state = State::Idle;
            currentWindow.reset();
            this->ISys().LogInfo("Reset: cleared queue and stopped current recording.");
        }
        else {
            this->ISys().LogInfo("Reset: queue cleared (no active recording).");
        }
        break;
    }
    case "Demo/Record/Start"_hash: {
        std::lock_guard<std::mutex> lock(mtx);
        moduleActive = true;
        this->ISys().LogInfo("Module activated.");
        break;
    }
    case "Demo/Record/Stop"_hash: {
        std::lock_guard<std::mutex> lock(mtx);
        moduleActive = false;

        if (state != State::Idle) {
            StopRecording();
            state = State::Idle;
            currentWindow.reset();
            this->ISys().LogInfo("Module deactivated, stopped current recording.");
        }
        else {
            this->ISys().LogInfo("Module deactivated.");
        }
        break;
    }
    default:
        break;
    }
}

// ========== 队列操作（无锁，调用者持有 mtx） ==========
bool DemoRecorder::PeekQueue(RecordTask& task) {
    if (recordTaskBufferQueue.empty()) return false;
    task = recordTaskBufferQueue.front();
    recordTaskBufferQueue.pop_front();
    return true;
}

// ========== 主循环 ==========
void DemoRecorder::Main() {
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (!moduleActive) {
            // 模块未激活时不做任何操作，任务循环会持续调用 Main，无需等待
            return;
        }

        switch (state) {
        case State::Idle: {
            RecordTask task;
            if (PeekQueue(task)) {
                currentWindow = task;
                StartRecording(task);
                state = State::Preparing;
            }
            break;
        }
        case State::Preparing: {
            // 前一帧已发送跳转/观察，本帧通知 OBS 开始录制
            MulNX::Message startMsg("Media/OBS/Record/Start"_hash);
            this->ISys().PublishAsync(std::move(startMsg));

            this->ISys().LogSucc("Recording started for UID="
                + std::to_string(currentWindow->uid)
                + " from tick " + std::to_string(windowStartTick)
                + " to " + std::to_string(windowEndTick));

            state = State::Recording;
            break;
        }
        case State::Recording: {
            int currentTick = this->CS2()->GetDemoTick();
            if (currentTick >= windowEndTick) {
                StopRecording();
                this->ISys().LogSucc("Recording finished for UID="
                    + std::to_string(currentWindow->uid));

                currentWindow.reset();
                state = State::Idle;
            }
            break;
        }
        }
    } // 释放锁，下一次 Main 调用由框架驱动
}

// ========== 录制控制 ==========
void DemoRecorder::StartRecording(const RecordTask& task) {
    windowStartTick = task.tick - preRecordTicks;
    windowEndTick = task.tick + postRecordTicks;

    if (windowStartTick < 0) {
        this->ISys().LogWarning("Window start tick adjusted from "
            + std::to_string(windowStartTick) + " to 0.");
        windowStartTick = 0;
    }

    MulNX::Message gotoMsg("Demo/GotoTick"_hash);
    gotoMsg.p1.low<int>() = windowStartTick;
    this->ISys().PublishAsync(std::move(gotoMsg));

    while (std::abs(this->CS2()->GetDemoTick() - windowStartTick) > 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    MulNX::Message specMsg("Observe/SpecSteam64UID"_hash);
    specMsg.p1.as<Steam64UID>() = task.uid;
    this->ISys().PublishAsync(std::move(specMsg));

    this->ISys().LogInfo("Jumped to tick " + std::to_string(windowStartTick)
        + ", observing UID=" + std::to_string(task.uid));
}

void DemoRecorder::StopRecording() {
    MulNX::Message stopMsg("Media/OBS/Record/Stop"_hash);
    this->ISys().PublishAsync(std::move(stopMsg));

    this->ISys().LogInfo("Sent OBS stop command.");
}