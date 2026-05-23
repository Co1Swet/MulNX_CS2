#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <deque>

class RecordTask {
public:
    std::string desc;
    Steam64UID uid;
    int tick;
};

class DemoJSONReader;
class DemoRecorder final : public CSModuleBase {
    DemoJSONReader* pJSON;

    std::deque<RecordTask> recordTaskBufferQueue;
    std::optional<RecordTask> currentRecordTask;
    int currentRecordTaskStartTick = 0;
    int currentRecordTaskEndTick = 0;

    std::atomic<bool> moduleActive = false;

    bool PeekQueue(RecordTask& task);   // 调用者需持有 mtx

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