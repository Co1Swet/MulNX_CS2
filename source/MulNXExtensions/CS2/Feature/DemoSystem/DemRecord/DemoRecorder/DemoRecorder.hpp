#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <deque>

class DemoRecorder final : public CSModuleBase, public MediaModuleMixin<DemoRecorder> {
    std::filesystem::path dirOutput{};
    std::string subOutput = "default";
    std::deque<RecordTask> recordTaskBufferQueue;

    std::atomic<bool> moduleActive = false;

    bool PeekQueue(RecordTask& task);
    void StartRecord();

    MulNX::CoTask WaitTimed(bool& flag, const float milliseconds, const std::function<bool()>& f);
    MulNX::CoTask Main();
    std::atomic<uint64_t> num = 0;
    void Window();
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};