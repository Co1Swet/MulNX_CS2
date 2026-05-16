#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <deque>

struct RecordToDo {
    Steam64UID uid = 0;
    int tick = 0;
};

class DemoRecorder final : public CSModuleBase {
    std::deque<RecordToDo> recordTaskBufferQueue;
    std::optional<RecordToDo> currentWindow;

    int windowStartTick = 0;
    int windowEndTick = 0;

    std::atomic<bool> moduleActive = false;

    bool PeekQueue(RecordToDo& task);   // 调用者需持有 mtx

    MulNX::CoTask coTa;
    MulNX::CoTask Main();
    bool Window(MulNX::UINode* node);
    std::atomic<bool>newStart = false;
    std::atomic<bool>isEmpty = false;
public:
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;

    // 配置参数：录制窗口相对于事件 tick 的前后偏移（单位：tick）
    int preRecordTicks = 300;
    int postRecordTicks = 300;
};