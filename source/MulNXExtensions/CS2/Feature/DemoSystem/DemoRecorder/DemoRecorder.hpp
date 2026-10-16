#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <deque>

class DemoRecorder final : public DemModuleBase, public MediaModuleMixin<DemoRecorder> {
    std::atomic<bool> moduleActive = false;
    fs::path dirOutput{};
    std::string subOutput = "default";

    std::deque<std::shared_ptr<Demo::RecordPass>> recordPasses{};

    std::atomic<std::shared_ptr<const Demo::RecordPass>> currentPass{};
    std::atomic<int> indexTask = -1;

    void StartRecord(const Demo::RecordTask& task);

    MulNX::CoTask EnsurePassChange();
    MulNX::CoTask GoNextTask(const Demo::RecordTask*& pTask);
    MulNX::CoTask Main();
    MulNX::CoTask RecordOne(const Demo::RecordTask& rTask);
    std::atomic<uint64_t> num = 0;
    void Window();
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};