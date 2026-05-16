#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <deque>

class IRecordTask {
public:
    virtual ~IRecordTask() = default;
    virtual Steam64UID GetTargetSteam64UID() = 0;
    virtual int GetTargetTick() = 0;
    virtual std::string& GetDesc() = 0;
};

class DemoJSONReader;
class DemoRecorder final : public CSModuleBase {
    DemoJSONReader* pJSON;

    std::deque<std::unique_ptr<IRecordTask>> recordTaskBufferQueue;
    std::optional<std::unique_ptr<IRecordTask>> currentRecordTask;
    int currentRecordTaskStartTick = 0;
    int currentRecordTaskEndTick = 0;

    std::atomic<bool> moduleActive = false;

    bool PeekQueue(std::unique_ptr<IRecordTask>& task);   // 调用者需持有 mtx

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