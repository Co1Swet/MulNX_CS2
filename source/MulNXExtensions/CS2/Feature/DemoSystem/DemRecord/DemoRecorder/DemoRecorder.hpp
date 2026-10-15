#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <deque>

class DemoRecorder final : public CSModuleBase, public MediaModuleMixin<DemoRecorder> {
    fs::path dirOutput{};
    std::string subOutput = "default";

    std::atomic<std::shared_ptr<const Demo::RecordPass>> currentPass{};
    std::atomic<int> indexTask = -1;
    std::deque<std::shared_ptr<Demo::RecordPass>> recordPasses{};

    std::atomic<bool> moduleActive = false;

    bool PeekQueue(Demo::RecordTask& task);
    void StartRecord(const Demo::RecordTask& task);

    MulNX::CoTask Main();
    MulNX::CoTask RecordOne(const Demo::RecordTask& rTask);
    std::atomic<uint64_t> num = 0;
    void Window();
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
};