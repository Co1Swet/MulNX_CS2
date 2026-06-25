#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Feature/DemoSystem/DemoStruct.hpp>
#include <deque>

class DemoRecorder final : public CSModuleBase {
    std::filesystem::path dirOutput{};
    std::string subOutput = "default";
    std::deque<RecordTask> recordTaskBufferQueue;
    std::optional<RecordTask> currentRecordTask;
    int currentRecordTaskStartTick = 0;
    int currentRecordTaskEndTick = 0;

    std::atomic<bool> moduleActive = false;

    bool PeekQueue(RecordTask& task);
    void StartRecord();

    MulNX::CoTask Main();
    std::atomic<uint64_t> num = 0;
    bool Window(MulNX::UINode* node);
public:
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};