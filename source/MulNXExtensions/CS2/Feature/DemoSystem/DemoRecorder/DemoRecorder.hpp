#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <deque>

class DemoRecorder final : public DemModule<DemoRecorder>, public MediaModuleMixin<DemoRecorder> {
    std::atomic<bool> moduleActive = false;
    fs::path dirOutput{};
    std::string subOutput = "default";

    std::deque<std::shared_ptr<Demo::Record::Pass>> recordPasses{};

    std::atomic<std::shared_ptr<const Demo::Record::Pass>> currentPass{};
    std::atomic<int> indexTask = -1;

    MulNX::CoTask CoSetOBTarget(bool& ok, Steam64UID target)const;

    MulNX::CoTask EnsurePassChange();
    MulNX::CoTask GoNextTask(std::optional<std::pair<std::shared_ptr<const Demo::Record::Pass>, size_t>>& out);
    MulNX::CoTask Main();
    MulNX::CoTask OnLoop();
    MulNX::CoTask RecordOne(const Demo::Record::Task& rTask, bool& recordSucc, const fs::path& outputDir)const;
    bool OnRecording(const Demo::Record::Task& curRecordTask, bool& recordSucc,
        std::optional<int>& observingMismatchSinceTick)const;

    void Window()const;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};