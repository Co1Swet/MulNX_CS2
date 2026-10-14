#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class RecordTaskConfiger;
class RecordTaskMaker final :public DemModuleBase {
    RecordTaskConfiger* pConfiger = nullptr;

    void PublishRecordTask(RecordTask&& rTask);

    bool showBekillEvent = true;
    std::shared_ptr<const Demo::Info> DemoChooseMenu(MulNX::UICoordinator* uico);
    void DemoMetaMenu(const Demo::Info& demoInfo);
    Steam64UID TeamPlayerMenu(const Demo::Info& demoInfo);
    void RoundPlayerMenu(const int& round, const Demo::Player& player, const Demo::Info& demoInfo);

    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
};