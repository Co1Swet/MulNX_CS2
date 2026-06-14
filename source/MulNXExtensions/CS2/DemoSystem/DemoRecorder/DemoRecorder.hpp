#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/DemoSystem/DemoStruct.hpp>
#include <deque>

class DemoRecorder final : public CSModuleBase {
    std::filesystem::path dirOutput{};
    std::string subOutput = "default";
    std::deque<RecordTask> recordTaskBufferQueue;
    std::optional<RecordTask> currentRecordTask;
    int currentRecordTaskStartTick = 0;
    int currentRecordTaskEndTick = 0;

    std::atomic<bool> moduleActive = false;

    bool PeekQueue(RecordTask& task);   // 调用者需持有 mtx

    MulNX::CoTask Main();
    std::atomic<uint64_t> num = 0;
    bool Window(MulNX::UINode* node);
public:
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};