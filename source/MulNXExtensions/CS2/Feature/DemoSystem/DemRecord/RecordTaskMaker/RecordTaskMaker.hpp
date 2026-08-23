#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class RecordTaskConfiger;
class RecordTaskMaker final :public DemModuleBase {
    RecordTaskConfiger* pConfiger = nullptr;
    std::map<std::string, Demo::Info>demos;

    void PublishRecordTask(RecordTask&& rTask);
    std::optional<RecordTask> CreatePlayFullRoundRTask(const int& round,
        const Demo::Player& player, const Demo::Info& demoInfo);

    bool showBekillEvent = true;
    bool DemoChooseMenu(Demo::Info*& pDemoInfo);
    void DemoMetaMenu(const Demo::Info& demoInfo);
    Steam64UID TeamPlayerMenu(const Demo::Info& demoInfo);
    void RoundPlayerMenu(const int& round, const Demo::PlayerRoundInfo& info,
        const Demo::Player& player, const Demo::Info& demoInfo);

    void Window(MulNX::UICoordinator* uico);
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;
};