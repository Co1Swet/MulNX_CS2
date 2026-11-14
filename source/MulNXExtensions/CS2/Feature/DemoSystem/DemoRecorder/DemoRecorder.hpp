#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <deque>

class DemoRecorder final : public DemModule<DemoRecorder>, public MediaModuleMixin<DemoRecorder> {
    std::atomic<bool> moduleActive = false;
    fs::path dirOutput{};
    std::string subOutput = "default";

    std::deque<std::shared_ptr<Demo::RecordPass>> recordPasses{};

    std::atomic<std::shared_ptr<const Demo::RecordPass>> currentPass{};
    std::atomic<int> indexTask = -1;
    mutable std::atomic<uint64_t> num = 0;

    void StartRecord(const Demo::RecordTask& task)const;
    MulNX::CoTask CoSetOBTarget(bool& ok, Steam64UID target)const;

    MulNX::CoTask EnsurePassChange();
    MulNX::CoTask GoNextTask(std::optional<std::pair<std::shared_ptr<const Demo::RecordPass>, size_t>>& out);
    MulNX::CoTask Main();
    MulNX::CoTask OnLoop();
    MulNX::CoTask RecordOne(const Demo::RecordTask& rTask, bool& recordSucc)const;
    bool OnRecording(const Demo::RecordTask& curRecordTask, bool& recordSucc,
        std::optional<int>& observingMismatchSinceTick)const;

    void Window()const;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};