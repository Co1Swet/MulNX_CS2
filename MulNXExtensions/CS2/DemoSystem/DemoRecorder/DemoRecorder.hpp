#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <string>

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoRecorder final : public CSModuleBase {
public:
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;

    // 配置参数：录制窗口相对于事件 tick 的前后偏移（单位：tick）
    int preRecordTicks = 800;
    int postRecordTicks = 256;

private:
    struct RecordTask {
        Steam64UID uid = 0;
        int tick = 0;
    };

    enum class State {
        Idle,
        Preparing,
        Recording
    };

    std::deque<RecordTask> recordTaskBufferQueue;
    std::optional<RecordTask> currentWindow;

    State state = State::Idle;
    int windowStartTick = 0;
    int windowEndTick = 0;

    bool moduleActive = true;
    mutable std::mutex mtx;

    bool PeekQueue(RecordTask& task);   // 调用者需持有 mtx

    void Main();
    void StartRecording(const RecordTask& task);
    void StopRecording();
};