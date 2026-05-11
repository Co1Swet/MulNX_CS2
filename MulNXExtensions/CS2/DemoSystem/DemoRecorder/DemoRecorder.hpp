#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <string>

#include <MulNX/Common/coroutine.hpp>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class DemoRecorder final : public CSModuleBase {
public:
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;

    // 配置参数：录制窗口相对于事件 tick 的前后偏移（单位：tick）
    int preRecordTicks = 800;
    int postRecordTicks = 256;

private:
    struct RecordToDo {
        Steam64UID uid = 0;
        int tick = 0;
    };


    std::deque<RecordToDo> recordTaskBufferQueue;
    std::optional<RecordToDo> currentWindow;

    int windowStartTick = 0;
    int windowEndTick = 0;

    bool moduleActive = true;

    bool PeekQueue(RecordToDo& task);   // 调用者需持有 mtx

    MulNX::CoTask coTa;
    MulNX::CoTask Main();
    void StopRecording();
};